#include <cstdlib>
#include <sys/fcntl.h>
#include <sys/event.h>
#include <netdb.h>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <map>
#include <deque>
#include <vector>
#include <signal.h>
#include <sys/stat.h>
#include <fstream>

#define LEN 2048


using std::deque;
using std::map;
using std::vector;
using response = std::function<void(struct kevent *)>;


int err(const char *msg) {
    std::cout << msg << std::endl;
    return 1;
}


int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }
    return 0;
}

int create_new_socket(int port) {
    sockaddr_in addr = {0};
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    auto sock = socket(PF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("sock error");
        return -1;
    }
    if (bind(sock, (sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind error");
        close(sock);
        return -1;
    }
    if (listen(sock, 10) < 0) {
        perror("listen error");
        close(sock);
        return -1;
    }
    if (set_nonblock(sock) < 0) {
        perror("nonblock error");
        close(sock);
        return -1;
    }
    return sock;
}


class Session {


private:


    int termFd;

    int clientFd;

    int serverFd;

    int termPs;


    int createNewTerminal() {

        int master_fd = posix_openpt(O_RDWR);
        if (master_fd < 0) {
            return err("master_fd");
        }
        if (grantpt(master_fd) < 0) {
            return err("grantpt");

        }
        if (unlockpt(master_fd) < 0) {
            return err("unlockpt");
        }


        int slave_fd = open(ptsname(master_fd), O_RDWR);
        if (slave_fd < 0) {
            return err("slave_fd");
        }

        pid_t id = fork();

        if (id == 0) {
            termios tattr;
            std::cout << "term opened" << std::endl;
            tcgetattr(slave_fd, &tattr);
            tattr.c_cc[VMIN] = 1;
            tattr.c_cc[VTIME] = 0;
            tattr.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(slave_fd, TCSANOW, &tattr);
            dup2(slave_fd, STDIN_FILENO);
            dup2(slave_fd, STDOUT_FILENO);
            dup2(slave_fd, STDERR_FILENO);
            close(master_fd);
            close(slave_fd);

            setsid();
            ioctl(0, TIOCSCTTY, 1);
            execlp("/bin/sh", "/bin/sh", NULL);
        }
        close(slave_fd);
        termFd = master_fd;
        termPs = id;
        return 0;
    }


public:
    Session() = delete;

    Session(int descriptor) : serverFd(descriptor) {
        sockaddr addr;
        socklen_t len = sizeof(sockaddr);
        clientFd = accept(descriptor, &addr, &len);
        set_nonblock(clientFd);
        const int a = 1;
        setsockopt(clientFd, SOL_SOCKET, SO_NOSIGPIPE, &a, sizeof(a));

        createNewTerminal();
    }

    bool isTermAlive() {
        int temp;
        return waitpid(termPs, &temp, WNOHANG) == 0;
    }

    int getClientSocket() const {
        return clientFd;
    }

    int getTermSocket() const {
        return termFd;
    }

    void stop() {
        close(termFd);
        close(clientFd);
        kill(termPs, SIGTERM);
    }


};

struct Holder {
    response read_response;
    response write_response;

    std::deque<char> inBuffer;
    std::deque<char> outBuffer;
};


class ConnListener {

public:
    int sock;

    ConnListener(int port) {

        sock = create_new_socket(port);
        listen(sock, 10);
    }

    Session *acceptClient() const {
        Session *client = new Session(sock);
        return client;
    }

    ~ConnListener() {
        close(sock);
    }

};


class Queue {

private:
    int kq;
    struct kevent evlist[1500];
    ConnListener listener;
    map<int, Holder *> holders;
    map<int, Session *> sessions;
public:

    Queue(int port) : kq(kqueue()), listener(port) {
        addServer(listener.sock, [this](struct kevent *kev) {
            this->addSession(listener.acceptClient());
        });
    }

    void addServer(int sock, response regFunc) {
        if (holders[sock] == nullptr)
            holders[sock] = new Holder();
        insert_event(sock, EVFILT_READ, regFunc);
    }


    void addSession(Session *session) {
        holders[session->getClientSocket()] = new Holder();
        sessions[session->getClientSocket()] = session;

        auto pt_fd = session->getTermSocket();
        insert_event(pt_fd, EVFILT_READ, [this, session, pt_fd](struct kevent * kev) {
            if (kev->flags & EV_EOF) {


            }

            vector<char> temp(LEN);
            int client_fd = session->getClientSocket();
            auto rec = read(pt_fd, temp.data(), LEN);
            if (rec > 0 && holders[client_fd] != nullptr) {
                auto inBuf = &holders[client_fd]->inBuffer;
                inBuf->insert(inBuf->end(), temp.begin(), temp.begin() + rec);
                insert_event(client_fd, EVFILT_WRITE, holders[client_fd]->write_response);
            }
        });

        insert_event(pt_fd, EVFILT_WRITE, [this, session, pt_fd](struct kevent *) {
            auto outbuf = &holders[session->getClientSocket()]->outBuffer;
            auto buf = std::string(outbuf->begin(), outbuf->end()).c_str();
            auto wr = write(pt_fd, buf, outbuf->size());
            if (wr > 0) {
                outbuf->erase(outbuf->begin(), outbuf->begin() + wr);
            }
            if (outbuf->size() == 0) {
                remove_event(pt_fd, EVFILT_WRITE);
            }
        });

        auto client_fd = session->getClientSocket();

        insert_event(client_fd, EVFILT_READ, [this, session, client_fd](struct kevent *event) {
            if (disconnect(event, EVFILT_READ)) return;

            Session *client = sessions[event->ident];
            if (client == nullptr) {
                return;
            }

            auto buf = &holders[event->ident]->outBuffer;
            vector<char> temp(LEN);
            ssize_t len = recv(client_fd, temp.data(), LEN, 0);

            if (len != -1) {
                buf->insert(buf->end(), temp.begin(), temp.begin() + len);
            }
            if (buf->size() > 0) {
                insert_event(client->getTermSocket(), EVFILT_WRITE, holders[client->getTermSocket()]->write_response);
            }
        });


        insert_event(client_fd, EVFILT_WRITE, [this, session, client_fd](struct kevent *event) {
            if (disconnect(event, EVFILT_WRITE)) return;

            Session *client = sessions[event->ident];
            if (client == nullptr) {
                return;
            }
            auto inBuf = &holders[event->ident]->inBuffer;
            std::string buf;
            ssize_t len;
            do {
                buf = std::string(inBuf->begin(), inBuf->end());
                len = send(client_fd, buf.c_str(), buf.size(), 0);

                if (len > 0) {
                    inBuf->erase(inBuf->begin(), inBuf->begin() + len);
                }
            } while (len != 0);

            if (inBuf->size() == 0) {
                remove_event(client->getClientSocket(), EVFILT_WRITE);
            }
        });
    }

    void dispatch() {
        while (true) {
            int num_events = kevent(kq, NULL, 0, evlist, SOMAXCONN, NULL);
            for (auto i = 0; i < num_events; ++i) {
                if (holders[evlist[i].ident] != nullptr) {
                    switch (evlist[i].filter) {
                        case EVFILT_READ: {
                            holders[evlist[i].ident]->read_response(&evlist[i]);
                            break;
                        }
                        case EVFILT_WRITE: {
                            holders[evlist[i].ident]->write_response(&evlist[i]);
                            break;
                        }
                        default:
                            continue;
                    }
                }
            }
        }
    }





    bool disconnect(struct kevent *event, int action) {
        if (event->flags & EV_EOF || !sessions[event->ident]->isTermAlive()) {
            auto client = sessions[event->ident];
            if (client != nullptr) {

                remove_event(client->getClientSocket(), action);
                remove_event(client->getTermSocket(), EVFILT_WRITE);
                delete holders[event->ident];
                delete holders[client->getTermSocket()];
                sessions.erase(event->ident);
                holders.erase(client->getTermSocket());
                holders.erase(event->ident);
                client->stop();
            }
            return true;
        }
        return false;
    }


    void remove_event(int fd, int filter) {
        notify(fd, filter, nullptr, EV_DELETE, 0, 0);
    }

    void insert_event(int fd, int filter, response handle) {
        notify(fd, filter, handle, EV_ADD, 0, 0);
    }


    void notify(int fd, int filter, response handle, int action, int flags, long long data) {
        struct kevent kev;
        if (holders[fd] == nullptr) {
            holders[fd] = new Holder();
        }
        if (handle != nullptr) {
            if (filter == EVFILT_READ) {
                holders[fd]->read_response = handle;
            }
            if (filter == EVFILT_WRITE) {
                holders[fd]->write_response = handle;
            }
        }

        EV_SET(&kev, fd, filter, action, flags, data, nullptr);
        if (kevent(kq, &kev, 1, 0, 0, 0) == -1) {
            std::cout << std::strerror(errno) << std::endl;
        }
    }

};


auto file = "/tmp/rshd.pid";

void clear(int signum, siginfo_t *info, void *context) {
    if (signum == SIGTERM || signum == SIGINT || signum == SIGHUP) {
        remove(file);
    }
}

int daemonize() {
    int fork_res = fork();
    if (fork_res > 0) {
        return 1;
    }
    setsid();
    chdir("/");
    umask(0);

    if (fork() > 0) {
        std::ofstream str(file, std::ofstream::trunc);
        str<<fork_res;
        str.close();
        return 1;
    }
    close(0);
    close(1);
    close(2);
    return 0;
}



int main(int argc, const char **argv) {
    if (daemonize()) {
        return 0;
    }
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGINT);
    act.sa_sigaction = &clear;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    Queue qu(atoi(argv[1]));
    qu.dispatch();
    return 0;
}