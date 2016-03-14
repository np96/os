#include <fcntl.h>


int main(int argc, const char ** argv) {
    int fd = 0;
    if (argc > 1) fd = open(argv[1], O_RDONLY);  
    int cct = 1;
    while (1) {
        char data[1024];
        int count = 0;
        int size = 1024;
        while (size > 0 && cct != 0) {
            void* tptr = data;
            int received = read(fd,tptr,size);
            if (received == -1 ) return 1;
            if (0 == received) cct = 0;
            tptr += received;
            count +=received;
            size-=received;
        }
        if (count == 0) return 0;
        int rec_count = 0;

        while (count > 0) {
            void* tptr = data;
            int sent = write(1,tptr,count);
            if (sent == -1) return 1;
            if (0 == sent) return 0;
            tptr += sent;
            rec_count +=sent;
            count -= sent;
        } 
    }
    if (fd != 0) close(fd);
    return 0;
    
}
