#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <signal.h>

#include <sys/syscall.h>
int flag = 0;

int readwrite(int fd) {
    int size = 1024;
    char data[size];
    int count;
    while ((count = read(0,data,size)) > 0) {

        int c = 0;
        while (count > c) {
            flag = 0;
            int received = write(fd,data+c,count-c);
            
            if (received == -1 ) {
                if (errno == EINTR) {
                    continue;
                } else return -1;
            }
            c += received;
        }
    }
    return count;
}

int main(int argc, const char ** argv) {
    int fd = 1;
    if (argc > 1) fd = open(argv[1], O_CREAT| O_RDWR, 0666);
    if (fd < 0) return 1;
    int i = EINTR;
    int res;
    while (i == EINTR) {
        res = readwrite(fd);
        i = errno;
    }
    if (fd != 1) close(fd);
    if (res < 0)  {
        return 1;
    }
    return 0;
}
