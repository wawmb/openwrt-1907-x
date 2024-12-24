#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

static int init_speed_arr[] = { B921600, B460800, B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B600, B300, B110 };
static const int init_name_arr[] = { 921600, 460800, 230400, 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 600, 300, 110 };
const int baud[] = { 921600, 460800, 230400, 115200, 57600, 38400, 19200, 9600 };

/************************************************************
 *	
 *	serial set baudrate
 *
 ************************************************************/
int serial_set_baudrate(struct termios* tms, int baudrate)
{
    struct termios* t = tms;
    int i;
    int ret = -1;

    if (tms == NULL) {
        return -1;
    }

    for (i = 0; i < sizeof(init_speed_arr) / sizeof(int); i++) {
        if (baudrate == init_name_arr[i]) {
            ret = cfsetispeed(t, init_speed_arr[i]);
            if (ret != 0) {
                printf("setispeed error\n");
                return -1;
            }

            ret = cfsetospeed(t, init_speed_arr[i]);
            if (ret != 0) {
                printf("setospeed error\n");
                return -1;
            }

            return 0;
        }
    }
    return -1;
}

static void printf_help(void)
{
    printf("eserial <device> <data> [baudrate] \n");

    printf("For example:    ./eserial /dev/ttyUSB4 at+csq 115200\n");
    printf("                ./eserial /dev/ttyUSB4 at\n");
}

int main(int argc, char* argv[])
{
    int device_fd;
    char command[256];
    int baudrate = 115200;

    int wrt;

    if (argc < 2) {
        printf_help();
        return -1;
    }

    device_fd = open(argv[1], O_RDWR);
    if (device_fd == -1) {
        printf("Can't Open Serial %s\n", argv[1]);
        printf_help();
        return -1;
    }

    if (argc >= 4) {
        baudrate = atoi(argv[3]);
    }
    struct termios s_opts;
    tcflush(device_fd, TCIOFLUSH);

    if (tcgetattr(device_fd, &s_opts) != 0) {
        printf("Get serial failed for %s!\n", argv[1]);
        return -1;
    }

    s_opts.c_lflag &= ~(ICANON | ECHO | ISIG);
    s_opts.c_oflag &= ~OPOST;

    cfsetispeed(&s_opts, B115200);
    serial_set_baudrate(&s_opts, baudrate);
    s_opts.c_cflag |= CS8;
    s_opts.c_cflag &= ~CSTOPB;
    s_opts.c_cflag &= ~PARENB;
    s_opts.c_cflag &= ~CRTSCTS;
    s_opts.c_cflag |= (CLOCAL | CREAD);
    s_opts.c_iflag &= ~INPCK;

    if (tcsetattr(device_fd, TCSANOW, &s_opts) != 0) {
        printf("Setup serial failed for %s!\n", argv[1]);
        return -1;
    }

    if (argc > 2) {
        if (strlen(argv[2]) > 252) {
            printf("AT command can not more than 252!\n");
            return -1;
        }

        memset(command, 0x00, sizeof(command));
        sprintf(command, "%s\r", argv[2]);

        wrt = write(device_fd, command, strlen(command));
        if (wrt == -1) {
            printf("Write error: %s\n", argv[1]);
            return -1;
        }

        while (1) {
            char recv_buf[5120];
            memset(recv_buf, 0x00, sizeof(recv_buf));
            ssize_t recv_size = read(device_fd, recv_buf, 5120);
            if (recv_size > 0) {
                recv_buf[recv_size] = 0;
                printf("%s", recv_buf);
                if (strstr(recv_buf, "\nOK") != 0)
                    break;
                if (strstr(recv_buf, "ERROR") != 0)
                    break;
            } else if (recv_size == -1) {
                printf("Read error: %s\n", argv[1]);
                return -1;
            }
        }
    } else {
        while (1) {
            char recv_buf[5120];
            memset(recv_buf, 0x00, sizeof(recv_buf));
            ssize_t recv_size = read(device_fd, recv_buf, 5120);
            if (recv_size > 0) {
                recv_buf[recv_size] = 0;
                printf("%s", recv_buf);
            } else if (recv_size == -1) {
                printf("Read error: %s\n", argv[1]);
                return -1;
            }
        }
    }
    return 0;
}