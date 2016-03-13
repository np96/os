#include <stdio.h>

int main() {
    
    int cct = 1;
    while (1) {
        char data[2048];
        int count = 0;
        int size = 1024;
	fflush(stdout);
        while (size > 0 && cct != 0) {
            fflush(stdout);
            void* tptr = data;
            int received = read(0,tptr,size);
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
    return 0;
    
}
