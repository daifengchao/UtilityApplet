#include "serial.h"

char *err_str[] = {
	"No error",
	"open com port fail",
	"invalid parameter",
	"get serial info fail",
	"set serial info fail",
	"send data fail",
	"read data fail",
	"read data timeout",
	"verify data fail"
};

struct test_opt mode_arr[] = {
	{MODE_RS232, "232"},
	{MODE_RS422, "422"},
/*	{MODE_RS485, "485"},*/
	{0, ""},
};

struct test_opt baudrate_arr[] = {
	{50, "50"},
	{75, "75"},
	{110, "110"},
	{150, "150"},
	{300, "300"},
	{600, "600"},
	{1200, "1200"},
	{1800, "1800"},
	{2400, "2400"},
	{4800, "4800"},
	{7200, "7200"},
	{9600, "9600"},
	{14400, "14400"},
	{19200, "19200"},
	{38400, "38400"},
	{57600, "57600"},
	{115200, "115200"},
	{230400, "230400"},
	{460800, "460800"},
	{921600, "921600"},
	{0, ""},
};

struct test_opt parity_arr[] = {
	{PARITY_NONE, "N"},
	{PARITY_EVEN, "E"},
	{PARITY_ODD, "O"},
	{PARITY_MARK, "M"},
	{PARITY_SPACE, "S"},
	{0, ""},
};

struct test_opt databit_arr[] = {
	{8, "8"},
	{7, "7"},
	{6, "6"},
	{5, "5"},
	{0, ""},
};

struct test_opt stopbit_arr[] = {
	{1, "1"},
	{2, "2"},
	{0, ""},
};

struct test_opt flowcontrol_arr[] = {
	{FC_NONE, "none"},
	{FC_RTSCTS, "RTS/CTS"},
	{FC_XONXOFF, "XOn/XOff"},
	{FC_BOTH, "Xon/XOff and RTS/CTS"},
	{0, ""},
};

int f_debug = 0;
int debug_printf(char *format, ...)
{
	va_list args;
	int ret = 0;
	if(f_debug) {
		va_start(args, format);
		ret = vprintf(format, args);
		va_end(args);
	}
	return ret;
}

void printTermios(struct termios *pSerialTermios)
{
//struct termios {
//	tcflag_t c_iflag;		/* input mode flags */
//	tcflag_t c_oflag;		/* output mode flags */
//	tcflag_t c_cflag;		/* control mode flags */
//	tcflag_t c_lflag;		/* local mode flags */
//	cc_t c_line;			/* line discipline */
//	cc_t c_cc[NCCS];		/* control characters */
//};
	int i;
	/* c_cc characters */
	char *c_ccs[NCCS] = {
	"VINTR",
	"VQUIT",
	"VERASE",
	"VKILL",
	"VEOF",
	"VTIME",
	"VMIN",
	"VSWTC",
	"VSTART",
	"VSTOP",
	"VSUSP",
	"VEOL",
	"VREPRINT",
	"VDISCARD",
	"VWERASE",
	"VLNEXT",
	"VEOL2"
	};

	printf("c_iflag:\n");
	if((pSerialTermios->c_iflag & IGNBRK) == IGNBRK)
		printf("IGNBRK ");
	if((pSerialTermios->c_iflag & BRKINT) == BRKINT)
		printf("BRKINT ");
	if((pSerialTermios->c_iflag & PARMRK) == PARMRK)
		printf("PARMRK ");
	if((pSerialTermios->c_iflag & INPCK) == INPCK)
		printf("INPCK ");
	if((pSerialTermios->c_iflag & ISTRIP) == ISTRIP)
		printf("ISTRIP ");
	if((pSerialTermios->c_iflag & INLCR) == INLCR)
		printf("INLCR ");
	if((pSerialTermios->c_iflag & IGNCR) == IGNCR)
		printf("IGNCR ");
	if((pSerialTermios->c_iflag & ICRNL) == ICRNL)
		printf("ICRNL ");
	if((pSerialTermios->c_iflag & IUCLC) == IUCLC)
		printf("IUCLC ");
	if((pSerialTermios->c_iflag & IXON) == IXON)
		printf("IXON ");
	if((pSerialTermios->c_iflag & IXANY) == IXANY)
		printf("IXANY ");
	if((pSerialTermios->c_iflag & IXOFF) == IXOFF)
		printf("IXOFF ");
	if((pSerialTermios->c_iflag & IMAXBEL) == IMAXBEL)
		printf("IMAXBEL ");
	printf("\n");

	printf("c_oflag:\n");
	if((pSerialTermios->c_oflag & OPOST) == OPOST)
		printf("OPOST ");
	if((pSerialTermios->c_oflag & OLCUC) == OLCUC)
		printf("OLCUC ");
	if((pSerialTermios->c_oflag & ONLCR) == ONLCR)
		printf("ONLCR ");
	if((pSerialTermios->c_oflag & OCRNL) == OCRNL)
		printf("OCRNL ");
	if((pSerialTermios->c_oflag & ONOCR) == ONOCR)
		printf("ONOCR ");
	if((pSerialTermios->c_oflag & ONLRET) == ONLRET)
		printf("ONLRET ");
	if((pSerialTermios->c_oflag & OFILL) == OFILL)
		printf("OFILL ");
	if((pSerialTermios->c_oflag & OFDEL) == OFDEL)
		printf("OFDEL ");
	if((pSerialTermios->c_oflag & NLDLY) == NLDLY)
		printf("NLDLY ");
	if((pSerialTermios->c_oflag & NL0) == NL0)
		printf("NL0 ");
	if((pSerialTermios->c_oflag & NL1) == NL1)
		printf("NL1 ");
	if((pSerialTermios->c_oflag & CRDLY) == CRDLY)
		printf("CRDLY ");
	if((pSerialTermios->c_oflag & CR0) == CR0)
		printf("CR0 ");
	if((pSerialTermios->c_oflag & CR1) == CR1)
		printf("CR1 ");
	if((pSerialTermios->c_oflag & CR2) == CR2)
		printf("CR2 ");
	if((pSerialTermios->c_oflag & CR3) == CR3)
		printf("CR3 ");
	if((pSerialTermios->c_oflag & TABDLY) == TABDLY)
		printf("TABDLY ");
	if((pSerialTermios->c_oflag & TAB0) == TAB0)
		printf("TAB0 ");
	if((pSerialTermios->c_oflag & TAB1) == TAB1)
		printf("TAB1 ");
	if((pSerialTermios->c_oflag & TAB2) == TAB2)
		printf("TAB2 ");
	if((pSerialTermios->c_oflag & TAB3) == TAB3)
		printf("TAB3 ");
	if((pSerialTermios->c_oflag & XTABS) == XTABS)
		printf("XTABS ");
	if((pSerialTermios->c_oflag & BSDLY) == BSDLY)
		printf("BSDLY ");
	if((pSerialTermios->c_oflag & BS0) == BS0)
		printf("BS0 ");
	if((pSerialTermios->c_oflag & BS1) == BS1)
		printf("BS1 ");
	if((pSerialTermios->c_oflag & VTDLY) == VTDLY)
		printf("VTDLY ");
	if((pSerialTermios->c_oflag & VT0) == VT0)
		printf("VT0 ");
	if((pSerialTermios->c_oflag & VT1) == VT1)
		printf("VT1 ");
	if((pSerialTermios->c_oflag & FFDLY) == FFDLY)
		printf("FFDLY ");
	if((pSerialTermios->c_oflag & FF0) == FF0)
		printf("FF0 ");
	if((pSerialTermios->c_oflag & FF1) == FF1)
		printf("FF1 ");
	printf("\n");

	printf("c_cflag:\n");
	if(pSerialTermios->c_cflag == B0)
		printf("B0 ");
	if((pSerialTermios->c_cflag & CBAUD) == B50)
		printf("B50 ");
	if((pSerialTermios->c_cflag & CBAUD) == B75)
		printf("B75 ");
	if((pSerialTermios->c_cflag & CBAUD) == B110)
		printf("B110 ");
	if((pSerialTermios->c_cflag & CBAUD) == B134)
		printf("B134 ");
	if((pSerialTermios->c_cflag & CBAUD) == B150)
		printf("B150 ");
	if((pSerialTermios->c_cflag & CBAUD) == B200)
		printf("B200 ");
	if((pSerialTermios->c_cflag & CBAUD) == B300)
		printf("B300 ");
	if((pSerialTermios->c_cflag & CBAUD) == B600)
		printf("B600 ");
	if((pSerialTermios->c_cflag & CBAUD) == B1200)
		printf("B1200 ");
	if((pSerialTermios->c_cflag & CBAUD) == B1800)
		printf("B1800 ");
	if((pSerialTermios->c_cflag & CBAUD) == B2400)
		printf("B2400 ");
	if((pSerialTermios->c_cflag & CBAUD) == B4800)
		printf("B4800 ");
	if((pSerialTermios->c_cflag & CBAUD) == B9600)
		printf("B9600 ");
	if((pSerialTermios->c_cflag & CBAUD) == B19200)
		printf("B19200 ");
	if((pSerialTermios->c_cflag & CBAUD) == B38400)
		printf("B38400 ");
	if((pSerialTermios->c_cflag & CSIZE) == CS5)
		printf("CS5 ");
	if((pSerialTermios->c_cflag & CSIZE) == CS6)
		printf("CS6 ");
	if((pSerialTermios->c_cflag & CSIZE) == CS7)
		printf("CS7 ");
	if((pSerialTermios->c_cflag & CSIZE) == CS8)
		printf("CS8 ");
	if((pSerialTermios->c_cflag & CSTOPB) == CSTOPB)
		printf("CSTOPB ");
	if((pSerialTermios->c_cflag & CREAD) == CREAD)
		printf("CREAD ");
	if((pSerialTermios->c_cflag & PARENB) == PARENB)
		printf("PARENB ");
	if((pSerialTermios->c_cflag & PARODD) == PARODD)
		printf("PARODD ");
	if((pSerialTermios->c_cflag & HUPCL) == HUPCL)
		printf("HUPCL ");
	if((pSerialTermios->c_cflag & CLOCAL) == CLOCAL)
		printf("CLOCAL ");
	if((pSerialTermios->c_cflag & CBAUD) == B57600)
		printf("B57600 ");
	if((pSerialTermios->c_cflag & CBAUD) == B115200)
		printf("B115200 ");
	if((pSerialTermios->c_cflag & CBAUD) == B230400)
		printf("B230400 ");
	if((pSerialTermios->c_cflag & CBAUD) == B460800)
		printf("B460800 ");
	if((pSerialTermios->c_cflag & CBAUD) == B500000)
		printf("B500000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B576000)
		printf("B576000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B921600)
		printf("B921600 ");
	if((pSerialTermios->c_cflag & CBAUD) == B1000000)
		printf("B1000000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B1152000)
		printf("B1152000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B1500000)
		printf("B1500000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B2000000)
		printf("B2000000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B2500000)
		printf("B2500000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B3000000)
		printf("B3000000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B3500000)
		printf("B3500000 ");
	if((pSerialTermios->c_cflag & CBAUD) == B4000000)
		printf("B4000000 ");
	if((pSerialTermios->c_cflag & CIBAUD) == CIBAUD)
		printf("CIBAUD ");
	if((pSerialTermios->c_cflag & CMSPAR) == CMSPAR)
		printf("CMSPAR ");
	if((pSerialTermios->c_cflag & CRTSCTS) == CRTSCTS)
		printf("CRTSCTS ");
	printf("\n");

	printf("c_lflag:\n");
	if((pSerialTermios->c_lflag & ISIG) == ISIG)
		printf("ISIG ");
	if((pSerialTermios->c_lflag & ICANON) == ICANON)
		printf("ICANON ");
	if((pSerialTermios->c_lflag & XCASE) == XCASE)
		printf("XCASE ");
	if((pSerialTermios->c_lflag & ECHO) == ECHO)
		printf("ECHO ");
	if((pSerialTermios->c_lflag & ECHOE) == ECHOE)
		printf("ECHOE ");
	if((pSerialTermios->c_lflag & ECHOK) == ECHOK)
		printf("ECHOK ");
	if((pSerialTermios->c_lflag & ECHONL) == ECHONL)
		printf("ECHONL ");
	if((pSerialTermios->c_lflag & NOFLSH) == NOFLSH)
		printf("NOFLSH ");
	if((pSerialTermios->c_lflag & TOSTOP) == TOSTOP)
		printf("TOSTOP ");
	if((pSerialTermios->c_lflag & ECHOCTL) == ECHOCTL)
		printf("ECHOCTL ");
	if((pSerialTermios->c_lflag & ECHOPRT) == ECHOPRT)
		printf("ECHOPRT ");
	if((pSerialTermios->c_lflag & ECHOKE) == ECHOKE)
		printf("ECHOKE ");
	if((pSerialTermios->c_lflag & FLUSHO) == FLUSHO)
		printf("FLUSHO ");
	if((pSerialTermios->c_lflag & PENDIN) == PENDIN)
		printf("PENDIN ");
	if((pSerialTermios->c_lflag & IEXTEN) == IEXTEN)
		printf("IEXTEN ");
	printf("\n");

	for(i = 0; i < NCCS; i++)
	{
		if(c_ccs[i])
			printf("c_cc[%s] = 0x%X\n", c_ccs[i], pSerialTermios->c_cc[i]);
		else
			printf("c_cc[%d] = 0x%X\n", i, pSerialTermios->c_cc[i]);
	}
}

int ConfigSerialPort(int PortHandle,
	int f_mode,
	int f_baudrate,
	int f_parity,
	int f_databit,
	int f_stopbit,
	int f_flowcontrol)
{
	int result = ERR_NONE;
	struct termios PortTermios;
	struct serial_struct serinfo;
	
	debug_printf("getting termios\n");
	tcgetattr(PortHandle, &PortTermios);

	PortTermios.c_cflag |= HUPCL | CLOCAL | CREAD;
	PortTermios.c_cflag &= ~CBAUD;
	PortTermios.c_cflag |= B38400;

	// clear up relative attributes we will set later
	PortTermios.c_cflag &= ~PARENB;
	PortTermios.c_cflag &= ~CMSPAR;
	PortTermios.c_cflag &= ~PARODD;
	PortTermios.c_iflag &= ~(INPCK);   /* disable input parity checking */

	switch(f_parity) {
	case PARITY_NONE:
		break;
	case PARITY_EVEN:
		PortTermios.c_cflag |= PARENB;
		PortTermios.c_iflag |= INPCK;      /* enable input parity checking */
		break;
	case PARITY_ODD:
		PortTermios.c_cflag |= PARENB | PARODD;
		PortTermios.c_iflag |= INPCK;      /* enable input parity checking */
 		break;
	case PARITY_MARK:
		PortTermios.c_cflag |= PARENB | CMSPAR | PARODD;
		PortTermios.c_iflag |= INPCK;      /* enable input parity checking */
		break;
	case PARITY_SPACE:
		PortTermios.c_cflag |= PARENB | CMSPAR;
		PortTermios.c_iflag |= INPCK;      /* enable input parity checking */
		break;
	default:
		result = ERR_PARAMETER;
		goto ERROR_END;
		break;
	}

	PortTermios.c_cflag &= ~CSIZE;
	switch(f_databit) {
	case 5:
		PortTermios.c_cflag |= CS5;
		break;
	case 6:
		PortTermios.c_cflag |= CS6;
		break;
	case 7:
		PortTermios.c_cflag |= CS7;
		break;
	case 8:
		PortTermios.c_cflag |= CS8;
		break;
	default:
		result = ERR_PARAMETER;
		goto ERROR_END;
		break;
	}

	switch(f_stopbit) {
	case 1:
		PortTermios.c_cflag &= ~CSTOPB;
		break;
	case 2:
		PortTermios.c_cflag |= CSTOPB;
		break;
	default:
		result = ERR_PARAMETER;
		goto ERROR_END;
		break;
	}

	switch(f_flowcontrol) {
	case FC_NONE:
		PortTermios.c_cflag &= ~CRTSCTS;
		PortTermios.c_iflag &= ~(IXON | IXOFF);      /* input/output flow */
		PortTermios.c_iflag &= ~IXANY;
		break;
	case FC_XONXOFF:
		PortTermios.c_cflag &= ~CRTSCTS;
		PortTermios.c_iflag |= (IXON | IXOFF);      /* input/output flow */
		PortTermios.c_iflag |= IXANY;
		break;
	case FC_RTSCTS:
		PortTermios.c_cflag |= CRTSCTS;
		PortTermios.c_iflag &= ~(IXON | IXOFF);     /* no sw flow ctrl */
		PortTermios.c_iflag |= IXANY;
		break;
	case FC_BOTH:
		PortTermios.c_cflag |= CRTSCTS;
		PortTermios.c_iflag |= (IXON | IXOFF);      /* input/output flow */
		PortTermios.c_iflag |= IXANY;
		break;
	default:
		result = ERR_PARAMETER;
		goto ERROR_END;
		break;
	}

	switch(f_mode) {
	case MODE_RS232:
		PortTermios.c_iflag &= ~IRS422;
		PortTermios.c_iflag &= ~IRS485;
		break;
	case MODE_RS422:
		PortTermios.c_iflag |= IRS422;
		PortTermios.c_iflag &= ~IRS485;
		break;
	case MODE_RS485:
		PortTermios.c_iflag &= ~IRS422;
		PortTermios.c_iflag |= IRS485;
		break;
	default:
		result = ERR_PARAMETER;
		goto ERROR_END;
		break;
	}

	PortTermios.c_lflag &= ~ECHO;
	PortTermios.c_lflag &= ~(ICANON);
	PortTermios.c_iflag &= ~ICRNL;

	PortTermios.c_oflag = 0;

	PortTermios.c_cc[VMIN] = 1;
	PortTermios.c_cc[VTIME] = 0;
	PortTermios.c_cc[VINTR] = 0;
	PortTermios.c_cc[VQUIT] = 0;
	PortTermios.c_cc[VSUSP] = 0;

	debug_printf("setting termios\n");
	tcsetattr(PortHandle, TCSANOW, &PortTermios);
	debug_printf("getting serial info\n");
	if (ioctl(PortHandle, TIOCGSERIAL, &serinfo) < 0) {
		printf("Cannot get serial info");
		result = ERR_GETSERIAL;
		goto ERROR_END;
	}

	serinfo.flags |= ASYNC_SPD_CUST;
	serinfo.custom_divisor = serinfo.baud_base / f_baudrate;

	debug_printf("setting serial info\n");
	if (ioctl(PortHandle, TIOCSSERIAL, &serinfo) < 0) {
		printf("Cannot set serial info");
		result = ERR_SETSERIAL;
		goto ERROR_END;
	}
	debug_printf("serial port ready\n");
ERROR_END:
	return result;
}


// the magic length and Test Data attempt to fulfill testing 5, 6 ,7 and 8 bit data length.
#define PatternLen 31
#define PatternGroup 3

int SimpleReadWriteTest(int WritePortHandle, int ReadPortHandle, int TestLen, int f_databit, int f_baudrate)
{
	fd_set fds;
	struct timeval waitTime;
	int ReadSize, ReadTotal, SendTotal, WrittenSize, i, j;
	unsigned char datamask = 0;
	char *TestData, *ReadData;
	int result = ERR_NONE;

	if(TestLen > MAX_DATA_LEN) {
		return ERR_PARAMETER;
	}
	
	ReadData = malloc(TestLen + 1);
	TestData = malloc(TestLen + 1);
	for(i = 0; i < f_databit; i++) {
		datamask |= (1 << i);
	}
	
	for(i = 0; i < TestLen; i++) {
		if(i == 0)
			j = 0;
		else if((i % PatternLen) == 0) {
			j++;
			if(j == PatternGroup)
				j = 0;
		}
		TestData[i] = (33 + (i % PatternLen) + (32 * j)) & datamask;
	}
	TestData[TestLen + 1] = '\0';
	
	SendTotal = 0;
	while(SendTotal < TestLen) {
		FD_ZERO(&fds);
		FD_SET(WritePortHandle, &fds);
		waitTime.tv_sec = 30;
		waitTime.tv_usec = 0;
		if(select(WritePortHandle + 1, 0, &fds, 0, &waitTime) > 0) {
   			WrittenSize = write(WritePortHandle, &TestData[SendTotal], TestLen - SendTotal);
	   		if(WrittenSize <= 0) {
	   			printf("Thread 1 write failed!\n");
	   			result = ERR_SEND;
	   			goto ERROR_END;
	   		}
		   	SendTotal += WrittenSize;
	   	}
	}

	ReadTotal = 0;
	while(ReadTotal < TestLen) {
		FD_ZERO(&fds);
		FD_SET(ReadPortHandle, &fds);
		waitTime.tv_sec = 10;
		waitTime.tv_sec = 0;
		waitTime.tv_usec = 0;
		waitTime.tv_usec = (((TestLen * 10 * 1000) / f_baudrate) + 100) * 2000;
		if(select(ReadPortHandle + 1, &fds, 0, 0, &waitTime) > 0) {
			ReadSize = read(ReadPortHandle, &ReadData[ReadTotal], TestLen - ReadTotal);
	   		if(ReadSize == 0)
	   			continue;
		   	if(ReadSize < 0) {
		   		printf("\nThread 2 Recv Failed!%d(%X)\n", ReadSize, ReadSize);
	   			result = ERR_READ;
	   			goto ERROR_END;
		   	}
		   	ReadTotal += ReadSize;
		} else {
   			result = ERR_READ_TIMEOUT;
   			goto ERROR_END;
		}
	}
	if(memcmp(ReadData, TestData, TestLen) != 0) {
		result = ERR_VERIFY;
	}
ERROR_END:
	free(TestData);
	free(ReadData);
	return result;
}
