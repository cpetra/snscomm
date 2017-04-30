/**************************************************************************
 Name        : comm.c
 Version     : 0.1
 Copyright (C) 2014 Constantin Petra
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
***************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <linux/limits.h>

int tty_fd;
typedef struct __st_data{
    float t;
    float v;
    int relay_on;
} st_data;

 enum {
     RUN_CONSOLE,
    RUN_DAEMON
 };

#define MAX_READS 200
static st_data reads[MAX_READS];
static char sock_name[PATH_MAX] = "/tmp/comm_socket";
//static char tty_name[PATH_MAX] = "/dev/ttyUSB0";
static int read_cnt = 0;
static int read_it = 0;
static int run_state = RUN_CONSOLE;
static int debug_mode = 0;
static pthread_t thr;
static pthread_mutex_t comm_mutex = PTHREAD_MUTEX_INITIALIZER;

static int init_serial(char *tty_name)
{
    struct termios tio;

    memset(&tio,0,sizeof(tio));
    tio.c_oflag = 0;
    tio.c_cflag = CS8 | CREAD | CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 5;

    tty_fd = open(tty_name, O_RDWR | O_NONBLOCK);
    if (tty_fd < 0) {
        return 0;
    }

    /* For now hardcoded to 19200. */
    cfsetospeed(&tio, B115200);
    cfsetispeed(&tio, B115200);

    tcsetattr(tty_fd, TCSANOW, &tio);

    return 1;
}

static int read_string(char *str, int max_size, int tmo)
{
    int ret;
    int cnt = 0;
    char c;
    int sleep_time = 0;

    memset(str, 0, max_size);

    do {
        ret = read(tty_fd, &c, 1) > 0;
        if (ret) {
            if (c == '\n') {
                break;
            }
            str[cnt] = c;
            if (++cnt >= max_size) {
                break;
            }

        }
        else {
            usleep(1000);
            /* one second no answer (or less) */
            if (++sleep_time > tmo) {
                return 0;
            }
        }
    } while (1);

    return 1;
}

static void print_usage(void)
{
    printf("Usage:\n");
    printf("   snscomm /dev/ttyUSBx\n");
    printf("   snscomm /dev/ttyUSBx cmd arg0 arg1 arg2\n");
    printf("   where cmd may be:\n");
    printf("      a - for reading current data\n");	   
    printf("      b - for setting specific paramters\n");
    printf("   if cmd is b then there are three arguments specified:\n");
    printf("      arg0 - integer, representing the channel ID\n");
    printf("      arg1 - integer, representing command to be transferred to remote device\n");
    printf("      arg2 - integer, representing command parameter to be transferred to remote device\n");
    printf(" Example:\n");
    printf("   snscomm /dev/ttyUSB1 b 1 0 50:\n");
    printf("    meaning to set device on channel 1 a reporting time of 50 * 8 seconds = 400 seconds (assuming a humidity sensor attached)\n");
    
}
int main(int argc, char *argv[])
{
    int s;
    char c = 'a';
    char res[64];
    int cnt = 0;
    char a;
    char *ttyname = "/dev/ttyUSB0";
    char bcmd[8];
    unsigned int v;
    
    if (argc > 1) {
	ttyname = argv[1];
    }
    else {
	print_usage();
	exit(0);
    }
    if (!init_serial(ttyname)) {
	printf ("init err\n");
        exit(EXIT_FAILURE);
    }

    tcflush(tty_fd, TCOFLUSH);
    cnt = 1;
    bcmd[0] = 'a';

    if (argc > 4) {
	bcmd[0] = 'b';
	bcmd[1] = (char)(atoi(argv[2]));
	bcmd[2] = (char)(atoi(argv[3]));
 
        v = atoi(argv[4]);
	bcmd[3] = (char)(v >> 8);
	bcmd[4] = (char)(v & 0xff);
	cnt = 5;

	printf("%s %s %s\n", argv[2], argv[3], argv[4]);
    }

    if (write(tty_fd, bcmd, cnt) != cnt) {
	printf("Err write\n");
	close(tty_fd);
        return 0;
    }
    
    while (read_string(res, sizeof(res) / sizeof(res[0]), 100) != 0)
    {
	printf("%s\n", res);
    }

    close(tty_fd);
    exit(EXIT_SUCCESS);
}

