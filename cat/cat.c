#include <fcntl.h>


int main(int argc, const char ** argv) {
    int fd = 0;
    if (argc > 1) fd = open(argv[1], O_RDONLY);  
    int size = 1024;
    char data[size];
    int count;
    while ((count = read(fd,data,size)) > 0) { 
        while (count > 0) {
	    int received = write(1,data+size-count,count);
            if (received == -1) return 1;  
            count -= received;
        } 
    }
    if (fd != 0) close(fd);
    if (count < 0) return 0;
    return 0;
    
}
