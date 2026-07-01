#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "accord-le-ioctl.h"

void usage(const char *argv0)
{
    fprintf(stderr, "%s - Set RTC date and time to accord-le rtc \n", argv0);
    fprintf(stderr, "Usage: %s [OPTIONS]\n", argv0);
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "\t-h, --help       print this message\n");
    fprintf(stderr, "\t-c, --csv        set date and time in YYYY:MM:DD:hh:mm:ss format\n");
    fprintf(stderr, "\t-t, --time       set time from ctime library\n");
    fprintf(stderr, "Example 1: %s  -c 2010:11:12:01:02:50 \n", argv0);
    fprintf(stderr, "Example 2: %s  -t \n", argv0);
}


void check_csv_time_format(char *optarg)
{
    /*Добавить проверку на крорректнсть формата*/
    printf("date = %s\n", optarg);
}

int get_int_from_csv_string(char *str, char **token)
{
    int num = 0;
    char *strnum = strtok_r(str, ":", token);
    num = atoi(strnum);
    return num;
}

void csv_time_to_acle_rtc_time(char *optarg, struct acle_rtc_time *rtc_time)
{
    char *tok;
    rtc_time->year = get_int_from_csv_string(optarg, &tok);
    rtc_time->month = get_int_from_csv_string(NULL, &tok);
    rtc_time->day = get_int_from_csv_string(NULL, &tok);
    rtc_time->hour = get_int_from_csv_string(NULL, &tok);
    rtc_time->minute = get_int_from_csv_string(NULL, &tok);
    rtc_time->second = get_int_from_csv_string(NULL, &tok);

}

void time_to_acle_rtc_time (struct acle_rtc_time *rtc_time) {
    time_t t = time (NULL);
    struct tm *l_time = localtime (&t);

    rtc_time->year = l_time->tm_year + 1900;
    rtc_time->month = l_time->tm_mon + 1;
    rtc_time->day = l_time->tm_mday;
    rtc_time->hour = l_time->tm_hour;
    rtc_time->minute = l_time->tm_min;
    rtc_time->second = l_time->tm_sec;

}

enum mode {
    CSV_M = 1 << 1,
    CTIME_M = 1 << 2
};

int main(int argc, char *argv[])
{
    struct acle_rtc_time rtc_time;
    int res = -1;
    int dev_fd = -1;
    int mode = 0;
    
    struct option options[] =
        {
            {"help", no_argument, NULL, 'h'},
            {"csv", required_argument, NULL, 'c'},
            {"time", no_argument, NULL, 't'},  // To use ctime library
            {NULL},
        };
    int optargc = 0;
    while (1)
        {
            int c = getopt_long(argc, argv, "hc:t", options, NULL);
            if (c == -1)
                {
                    break;
                }
            optargc++;
            switch (c)
                {
                    case 'c':
                        if (mode & CTIME_M)  break;
                        mode = CSV_M;
                        check_csv_time_format(optarg);
                        memset(&rtc_time, 0, sizeof(rtc_time));
                        csv_time_to_acle_rtc_time(optarg, &rtc_time);
                        break;
                    case 't':
                        if (mode & CSV_M)   break;
                        mode = CTIME_M;
                        time_to_acle_rtc_time (&rtc_time);
                        break;

                    case 'h':
                    default:
                        usage(argv[0]);
                        exit(1);
                }
        }
 
    if (optargc < 1) 
        {
            usage(argv[0]);
            exit(1);
        }


    dev_fd = open("/dev/accord-le", O_RDWR);
    if(dev_fd == -1)
    	{
            fprintf(stderr, "Error: failed to open device: %m\n");
            goto out;
        }

    res = ioctl(dev_fd, ACLE_CMD_SET_RTC, &rtc_time);

    if(res)
        {
            fprintf(stderr, "ioctl failed: %m\n");
            goto out;
        }

    res = 0;
out:
    close(dev_fd);
    return res;
}
