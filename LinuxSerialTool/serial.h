#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>

#ifdef __uClinux__
#include <asm/termbits.h>
#include <linux/tty.h>
#include <sys/timeb.h> /* timeb struct, ftime function */
#else
#include <time.h> /* POSIX terminal control definitions */
#include <termios.h> /* POSIX terminal control definitions */
#endif

#include <linux/net.h>
#include <linux/ip.h>
#include <netinet/tcp.h>
#include <linux/udp.h>
#include <linux/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define IRS422	0100000
#define IRS485	0200000

#ifndef CMSPAR
#define CMSPAR    010000000000		/* mark or space (stick) parity */
#endif

#ifndef CDTRDSR
#define CDTRDSR   004000000000
#endif

#define MAX_DATA_LEN	4096

enum errcode {
	ERR_NONE = 0,
	ERR_OPEN,
	ERR_PARAMETER,
	ERR_GETSERIAL,
	ERR_SETSERIAL,
	ERR_SEND,
	ERR_READ,
	ERR_READ_TIMEOUT,
	ERR_VERIFY,
};

enum parity {
	PARITY_NONE = 1,
	PARITY_EVEN,
	PARITY_ODD,
	PARITY_MARK,
	PARITY_SPACE
};

enum flowcontrol {
	FC_NONE = 1,
	FC_XONXOFF,
	FC_RTSCTS,
	FC_DTRDSR,//jinxin added
	FC_BOTH
};

enum mode {
	MODE_RS232 = 1,
	MODE_RS422,
	MODE_RS485
};

struct test_opt {
	long val;
	char *str;
};

void printTermios(struct termios *pSerialTermios);
#define SHOW_HERE() \
	printf("%s:%d\n", __FUNCTION__, __LINE__); \

extern char *err_str[];
extern struct test_opt mode_arr[];
extern struct test_opt baudrate_arr[];
extern struct test_opt parity_arr[];
extern struct test_opt databit_arr[];
extern struct test_opt stopbit_arr[];
extern struct test_opt flowcontrol_arr[];

extern int f_debug;

extern int debug_printf(char *format, ...);
extern int ConfigSerialPort(int PortHandle, int f_mode, int f_baudrate, int f_parity,
	int f_databit, int f_stopbit, int f_flowcontrol);
extern int SimpleReadWriteTest(int WritePortHandle, int ReadPortHandle, 
	int TestLen, int f_databit, int f_baudrate);

#endif
