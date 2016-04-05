#include <unistd.h>
#include <signal.h>
#include <string>
#include <deque>
#include <sstream>
#include <stdlib.h>
#include <stdbool.h>
#include <iostream>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>


void handler(int signo) {
    if (signo == SIGINT) {
        kill(pids)
    }
    
}

std::deque<std::string> &split(const std::string &s, char delim, std::deque<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::string readline() {
    int n;
    int size = 1024;
    char buf[size];
    std::string s = "";
    
    while ((n = read(0, buf, size)) > 0) {
        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n' || buf[i] == '\0')
                return s+'\0';
            s += buf[i];
        }
        
    }
    
    return s;
}


void proceedCommand(std::string tokens) {
    std::deque<std::string> commandList(0);
    commandList = split(tokens, ' ', commandList);
    const int len = commandList.size() + 1;
    char * arguments[len];
    for (int i = 0; i < commandList.size() ; ++i) {
        arguments[i] = (char *) commandList[i].c_str();
    }
    arguments[len -1] = NULL;
    execvp(arguments[0], arguments);
}


int main(int argc, char ** argv) {
    signal(SIGINT, handler);
    while (1) {
        std::string args;
        args = readline();
        std::deque<std::string> tokens;
        tokens = split(args, '|', tokens);
        std::deque<int> pids(tokens.size());
        std::deque<int*> ffd(tokens.size(), new int[2]);
        for (int i = 0; i < tokens.size() - 1; ++i) {
            pipe(ffd[i]);
        }
        for (int i = 0; i < tokens.size(); ++i) {
            int id;
            if ((id =fork()) > 0) {
                pids[i] = id;
            } else {
                if (i != 0) {
                    dup2(ffd[i - 1][0], STDIN_FILENO);
                    close(ffd[i-1][1]);
                }
                if (i != tokens.size() - 1) {
                    dup2(ffd[i][1], STDOUT_FILENO);
            }
                close(ffd[i][0]);
                proceedCommand(tokens[i]);
            }
            if (id < 0) return 0;
        }
        for (int i = 0; i < tokens.size() - 1; ++i ){
            close(ffd[i][0]);
            close(ffd[i][1]);
        }
        for (int i = 0, j; i < tokens.size(); ++i) {
            waitpid(pids[i], &j, 0);
        }
        pids.clear();
    }
}