/*
 * compile on / for target
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TCSDEV "/dev/tcs0"

struct tcs3200_measurement {
        uint32_t white_head;
        uint32_t red;
        uint32_t green;
        uint32_t blue;
        uint32_t white_tail;
};

int main(void) {
        int fd;
        struct tcs3200_measurement m;

        printf("hello master\n");
        fd = open(TCSDEV, O_RDONLY);
        if(fd < 0) {
                perror("open failed");
                return -1;
        }
        while(1) {
                if(read(fd, &m, sizeof(struct tcs3200_measurement)) == sizeof(struct tcs3200_measurement)) {
                        printf("white head:  %u\n", m.white_head);
                        printf("red:         %u\n", m.red);
                        printf("green:       %u\n", m.green);
                        printf("blue:        %u\n", m.blue);
                        printf("white tail   %u\n", m.white_tail);
                }
                printf("\n");
                sleep(1);
        }
        return 0;
}


