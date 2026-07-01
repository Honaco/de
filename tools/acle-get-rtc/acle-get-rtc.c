#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>


#include "accord-le-ioctl.h"

void usage(const char *argv0)
{
    fprintf(stderr, "%s - Print data and time from accord-le rtc \n", argv0);
    fprintf(stderr, "Usage: %s [OPTIONS]\n", argv0);
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-h, --help       print this message\n");
    fprintf(stderr, "\t-c, --csv        print date and time in YYYY:MM:DD:hh:mm:ss format\n");
}



void print_acle_rtc_time(struct acle_rtc_time *rtc_time)
{
    printf("\nDate\n");
    printf("\tYear : %d\n", rtc_time->year);
    printf("\tMonth : %d\n", rtc_time->month);
    printf("\tDay : %d\n", rtc_time->day);
    printf("\nTime\n");
    printf("\tHour : %d\n", rtc_time->hour);
    printf("\tMinutes: %d\n", rtc_time->minute);
    printf("\tSecond : %d\n", rtc_time->second);

}

void print_acle_rtc_time_csv(struct acle_rtc_time *rtc_time)
{
    printf("%d:%d:%d:%d:%d:%d\n", 
            rtc_time->year,
            rtc_time->month,
            rtc_time->day,
            rtc_time->hour,
            rtc_time->minute,
            rtc_time->second);

}



int main(int argc, char *argv[])
{
    struct acle_rtc_time rtc_time;
    int res = -1;
    int dev_fd = -1;
    bool csv_mod = false;
    
    struct option options[] =
        {
            {"help", optional_argument, NULL, 'h'},
            {"csv", optional_argument, NULL, 'c'},
            {NULL},
        };
    while (1)
        {
            int c = getopt_long(argc, argv, "ch", options, NULL);
            if (c == -1)
                {
                    break;
                }
            switch (c)
                {
                    case 'c':
                        csv_mod = true;
                        break;
                    case 'h':
                    default:
                        usage(argv[0]);
                        exit(1);
                }
        }


    memset(&rtc_time, 0, sizeof(rtc_time));


    dev_fd = open("/dev/accord-le", O_RDWR);
    if(dev_fd == -1)
    	{
            fprintf(stderr, "Error: failed to open device: %m\n");
            goto out;
        }

    res = ioctl(dev_fd, ACLE_CMD_GET_RTC, &rtc_time);

    if(res)
        {
            fprintf(stderr, "ioctl failed: %m\n");
            goto out;
        }
    if (csv_mod)
        {
            print_acle_rtc_time_csv(&rtc_time);
        }
    else
        {
            print_acle_rtc_time(&rtc_time);
        }

    res = 0;
out:
    close(dev_fd);
    return res;
}
