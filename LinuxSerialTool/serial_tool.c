#include "serial.h"

#define RECV_TIMEOUT	3  // ms
enum interaction {
	IA_ACTIVE = 0, // active will send anyway
	IA_PASSIVE,	// passive will send after first incoming character
	IA_LOOP
};

int f_send = 0, f_recv = 0, baudrate = 230400;
int f_verify = 0, f_output = 0;
int f_dataerror = 0;
enum interaction f_interaction = IA_ACTIVE;
enum parity f_parity = PARITY_NONE;
int databit = 8, stopbit = 1;
enum flowcontrol f_flowcontrol = FC_NONE;
enum mode f_mode = MODE_RS232;
int pause_period = 0;
int packet_size = MAX_DATA_LEN;
int bRunning = 0;
int f_firstchar = 0;
int f_stopOnError = 0;
int stop_time = 10;

unsigned char datamask = 0xFF;
unsigned long long SendThroughput = 0;
unsigned long long RecvThroughput = 0;
unsigned long long SendTotal = 0;
unsigned long long RecvTotal = 0;
unsigned long long SendElapsed = 0;
unsigned long long RecvElapsed = 0;
unsigned char WriteData[MAX_DATA_LEN];
int basechar = 0x13;
int stepcount = 15;
int timeoutcount = 0;
struct timespec starttime;
struct timespec nowtime;
struct timespec recvstarttime;
struct timespec recvnowtime;
struct timespec sendstarttime;
struct timespec sendnowtime;
struct serial_icounter_struct icount;

char *ProgName = 0;

void StopTime(void)
{
	sleep(stop_time);
	bRunning = 0;	
}

void *SendThread(void *param)
{
	fd_set fds;
	struct timeval waitTime;
	int WrittenSize = 0, Size;
	unsigned char flagstart = 0;
	struct timespec sendstarttime;
	struct timespec sendnowtime;
	int SendPortHandle = (int)param;
	int sleep_period = pause_period * 1000;

	while(bRunning) {
		if(f_interaction && !f_firstchar) {
			usleep(10000);
			continue;
		}

		FD_ZERO(&fds);
		FD_SET(SendPortHandle, &fds);
		waitTime.tv_sec = 0;
//		waitTime.tv_usec = RECV_TIMEOUT * 100000;
		waitTime.tv_usec = (((packet_size * 10 * 1000) / baudrate) + 10) * 1000;
		if(select(SendPortHandle + 1, 0, &fds, 0, &waitTime) > 0) {
			if(!flagstart) {
				clock_gettime(CLOCK_REALTIME, &sendstarttime);
				flagstart = 1;
			}

   			Size = write(SendPortHandle,
   				&WriteData[WrittenSize],
   				packet_size - WrittenSize);
	   		if(Size < 0) {
	   			printf("Thread 1 write failed!\n");
	   			goto ENDSENDCLOSE;
	   		}
			if(Size != packet_size)
				debug_printf("SEND:%d\n", Size);
//			tcdrain(SendPortHandle);

		   	SendTotal += Size;
			clock_gettime(CLOCK_REALTIME, &sendnowtime);
			SendElapsed = sendnowtime.tv_sec - sendstarttime.tv_sec;
			if(SendElapsed > 0)
				SendThroughput = SendTotal/SendElapsed;

			WrittenSize += Size;
			if(WrittenSize == packet_size)
				WrittenSize = 0;

			if(sleep_period)
				usleep(sleep_period);
	   	} else
	   		timeoutcount++;
	}

ENDSENDCLOSE:
	f_send = 0;
	tcflush(SendPortHandle, TCOFLUSH);
	pthread_exit(0);
	return 0;
}

void *RecvThread(void *param)
{
	int ReadSize = 0;
	fd_set fds;
	struct timeval waitTime;
	char ReadData[MAX_DATA_LEN];
	int RecvPortHandle = (int)param;
	int ReadIndex, i;

	while(bRunning) {
		memset(ReadData, 0, MAX_DATA_LEN);
		FD_ZERO(&fds);
		FD_SET(RecvPortHandle, &fds);
		waitTime.tv_sec = 0;
		waitTime.tv_usec = RECV_TIMEOUT * 100000;
		if(select(RecvPortHandle + 1, &fds, 0, 0, &waitTime) > 0) {
			if(!f_firstchar) {
				clock_gettime(CLOCK_REALTIME, &recvstarttime);
				f_firstchar = 1;
			}

	   		ReadSize = read(RecvPortHandle, &ReadData[0], packet_size);
	   		if(ReadSize == 0)
	   			continue;
		   	if(ReadSize < 0) {
		   		printf("Thread 2 Recv Failed!\n");
		   		goto ENDRECVCLOSE;
		   	}
			//printf("ReadSize:%d\n", ReadSize);
			if(f_verify) {
			   	ReadIndex = 0;
			   	do {
			   		if(ReadData[ReadIndex] !=
			   			((basechar + (RecvTotal % packet_size % stepcount)) & datamask)) {
			   			printf("%u, %X, %X\n",
			   				RecvTotal, ReadData[ReadIndex],
			   				(basechar + (RecvTotal % packet_size % stepcount)) & datamask);
			   			printf("ReadSize:%d\n", ReadSize);
		   				for(i = 0;i < ReadSize; i++) {
		   					printf("%02X ",
		   					ReadData[i]);
		   					if((i % 20) == 19)
		   						printf("\n");
		   				}
		   				printf("\n");
		   				fflush(stdout);
			   			f_dataerror = 1;
			   			f_verify = 0;
			   			if(f_stopOnError && f_dataerror) {
			   				bRunning = 0;
			   			}
			   		}
			   		ReadIndex++;
			   		RecvTotal++;
			   	} while ((ReadSize != ReadIndex) && f_verify);
			 } else {
				RecvTotal += ReadSize;
			}

			clock_gettime(CLOCK_REALTIME, &recvnowtime);
			RecvElapsed = recvnowtime.tv_sec - recvstarttime.tv_sec;
			if(RecvElapsed > 0)
				RecvThroughput = RecvTotal/RecvElapsed;
		}
	}

ENDRECVCLOSE:
	f_recv = 0;
	pthread_exit(0);
	return 0;
}

void *ShowThread(void *param)
{
	int PortHandle = (int)param;

	while(bRunning) {
		if (ioctl(PortHandle, TIOCGICOUNT, &icount) < 0) {
			printf("Cannot get interrupt counters");
			close(PortHandle);
			goto END;
		}
		clock_gettime(CLOCK_REALTIME, &nowtime);
		printf("\rR %llu, %llu BPS<= (%02u:%02u:%02u) =>S %llu, %llu BPS, %d%5s",
			RecvTotal, RecvThroughput,
			(nowtime.tv_sec - starttime.tv_sec) / (60 * 60),
			((nowtime.tv_sec - starttime.tv_sec) % (60 * 60)) / 60,
			(nowtime.tv_sec - starttime.tv_sec) % 60,
			SendTotal, SendThroughput,
			timeoutcount,
			f_dataerror?", data error":"");
		fflush(stdout);
		usleep(500000);
	}

END:
	f_output = 0;
	pthread_exit(0);
	return 0;
}

void *LoopThread(void *param)
{
	int ReadSize, SendTotal, WrittenSize;
	fd_set fds;
	struct timeval waitTime;
	char ReadData[MAX_DATA_LEN];
	int PortHandle = (int)param;

	while(bRunning) {
		memset(ReadData, 0, MAX_DATA_LEN);
		FD_ZERO(&fds);
		FD_SET(PortHandle, &fds);
		waitTime.tv_sec = 0;
		waitTime.tv_usec = RECV_TIMEOUT * 100000;
		if(select(PortHandle + 1, &fds, 0, 0, &waitTime) > 0) {
	   		ReadSize = read(PortHandle, &ReadData[0], MAX_DATA_LEN);
	   		if(ReadSize == 0)
	   			continue;
		   	if(ReadSize < 0) {
		   		printf("Thread 2 Recv Failed!%d(%X)\n", ReadSize, ReadSize);
		   		goto ENDCLOSE;
		   	}

			SendTotal = 0;
			while(SendTotal != ReadSize) {
				FD_ZERO(&fds);
				FD_SET(PortHandle, &fds);
				waitTime.tv_sec = 0;
				waitTime.tv_usec = RECV_TIMEOUT * 100000;
				if(select(PortHandle + 1, 0, &fds, 0, &waitTime) > 0) {
		   			WrittenSize = write(PortHandle,
		   				&ReadData[SendTotal],
		   				ReadSize - SendTotal);
			   		if(WrittenSize < 0) {
			   			printf("Thread 1 write failed!\n");
			   			goto ENDCLOSE;
			   		}
				   	SendTotal += WrittenSize;
			   	}
			}
		}
	}

ENDCLOSE:
	pthread_exit(0);
	return 0;
}

void *RS485Thread(void *param)
{
	int ReadSize, ReadTotal, SendTotal, WrittenSize;
	fd_set fds;
	struct timeval waitTime;
	char ReadData[MAX_DATA_LEN];
	int PortHandle = (int)param;
	int sleep_period = pause_period * 1000;
	int transnit_timeout 
		= ((packet_size * 10 * 1000) / baudrate) + 10; //ms, 10 for delay
	// n81 to be 10;
	// 1s = 1000 ms;

	debug_printf("RS485Thread running!\n");
	if(f_interaction == IA_ACTIVE) {
		SendTotal = 0;
		FD_ZERO(&fds);
		FD_SET(PortHandle, &fds);
		waitTime.tv_sec = 0;
		waitTime.tv_usec = RECV_TIMEOUT * 100000;
		if(select(PortHandle + 1, 0, &fds, 0, &waitTime) > 0) {
   			WrittenSize = write(PortHandle,
   				&WriteData[SendTotal % packet_size],
   				packet_size * 2 - SendTotal);
	   		if(WrittenSize <= 0) {
	   			printf("RS485Thread write failed!\n");
	   			goto ENDCLOSE;
	   		}
		   	SendTotal += WrittenSize;
//		   	tcdrain(PortHandle);
	   	}
	}

	while(bRunning) {
		ReadTotal = 0;
		while(ReadTotal < packet_size) {
			memset(ReadData, 0, MAX_DATA_LEN);
			FD_ZERO(&fds);
			FD_SET(PortHandle, &fds);
			if(f_interaction == IA_PASSIVE) {
				waitTime.tv_sec = 100;
				waitTime.tv_usec = 0;
			} else {
				waitTime.tv_sec = 0;
				waitTime.tv_usec = transnit_timeout * 1000 * 2;
				// double transmittion time
			}
			if(select(PortHandle + 1, &fds, 0, 0, &waitTime) > 0) {
	   			if(!f_firstchar) {
					clock_gettime(CLOCK_REALTIME, &recvstarttime);
					f_firstchar = 1;
				}

				ReadSize = read(PortHandle,
					&ReadData[ReadTotal],
					packet_size - ReadTotal);
		   		if(ReadSize == 0)
		   			continue;
			   	if(ReadSize < 0) {
			   		printf("\nThread 2 Recv Failed!%d(%X)\n",
			   			ReadSize, ReadSize);
			   		goto ENDCLOSE;
			   	}
			   	ReadTotal += ReadSize;

			   	RecvTotal += ReadSize;
				if(f_output) {
					clock_gettime(CLOCK_REALTIME, &recvnowtime);
					RecvElapsed = recvnowtime.tv_sec - recvstarttime.tv_sec;
					if(RecvElapsed)
						RecvThroughput = RecvTotal/RecvElapsed;
				}
			} else {
				if(f_interaction == IA_ACTIVE) {
					timeoutcount++;
					break;
				}
			}
		}
//		usleep(2 * 1000);
		if(sleep_period)
			usleep(sleep_period);
		SendTotal = 0;
		while(SendTotal < packet_size) {
			FD_ZERO(&fds);
			FD_SET(PortHandle, &fds);
			waitTime.tv_sec = 0;
			waitTime.tv_usec = RECV_TIMEOUT * 100000;
			if(select(PortHandle + 1, 0, &fds, 0, &waitTime) > 0) {
	   			WrittenSize = write(PortHandle,
	   				&WriteData[SendTotal],
	   				packet_size - SendTotal);
		   		if(WrittenSize < 0) {
		   			printf("Thread 1 write failed!\n");
		   			goto ENDCLOSE;
		   		}
			   	SendTotal += WrittenSize;
		   	}
		}
	}

ENDCLOSE:
	pthread_exit(0);
	return 0;
}

void usage(void)
{
	printf("usage:\t %s serial-device -sSrRbBcCfFvVoOdDaApPlLmMzZkK "
		"[arg] ... \n\n", ProgName);
	printf("%s is a simple serial test program for Advantech Embedded ",ProgName);
	printf("Linux System.\n");
	printf("-s, --send\n");
	printf("\tsend specific pattern data to serial port\n");
	printf("-r, --receive\n");
	printf("\treceive data from serial port\n");
	printf("-b, --baudrate\n");
	printf("\tSet using baudrate\n");
	printf("-c, --communication\n");
	printf("\tSet communication parameter [parity | databit | stopbit]\n");
	printf("\t\tParity can be following value\n");
	printf("\t\t\t'n' means no parity\n");
	printf("\t\t\t'e' means even parity\n");
	printf("\t\t\t'o' means odd parity\n");
	printf("\t\t\t'm' means mark parity\n");
	printf("\t\t\t's' means space parity\n");
	printf("\t\tDatabit can be 5, 6, 7 or 8\n");
	printf("\t\tStopbit can be 1 or 2\n");
	printf("\t\tStopbit will be 1.5 actually, when databit is 5\n");
	printf("\t\tn81 means no parity, 8 databit, 1 stopbit\n");
	printf("Press ENTER to continue...\n");
	getchar();
//	StopTime(); //add by tingting.wu
	bRunning = 0;	
	printf("-f, --flowcontrol\n");
	printf("\tFlow control can be following value\n");
	printf("\tnone means no flow control\n");
	printf("\txonxoff means XOn/XOff software flow control\n");
	printf("\trtscts means RTS/CTS hardware flow control\n");
	printf("\tdtrdsr means DTR/DSR hardware flow control\n");
	printf("\tboth means use XOn/XOff and RTS/CTS flow control\n");
	printf("-v, --verify\n");
	printf("\tVerify that receive data is the same with specific pattern or not\n");
	printf("-o, --output\n");
	printf("\tOutput statistic data\n");
	printf("-d, --debug\n");
	printf("\tOutput debug messages\n");
	printf("-a, --active\n");
	printf("-p, --passive\n");
	printf("\tPassive side will wait for the first character input from active side\n");
  	printf("Press ENTER to continue...\n");
        getchar();
//	StopTime();
	printf("-l, --loopback\n");
	printf("\tSend received data back to original serial port\n");
	printf("-m, --mode\n");
	printf("\tTransmit mode will be 232, 422 or 485\n");
	printf("-k, --packet\n");
	printf("\tTransmit packet size\n");
	printf("-z, --pause\n");
	printf("\tTransmit will pause for specific ms per packet\n");
	printf("-t, --stop\n");
	printf("\tStop on error.\n");
	printf("-i, --daemon\n");
	printf("\tRun test as a daemon in the back-ground.\n");
	printf("-h");
	printf("\tTimeout\n");

	exit(0);
}

static struct option long_options[] = {
	{"send", no_argument, 0, 's'},
	{"receive", no_argument, 0, 'r'},
	{"baudrate", required_argument, 0, 'b'},
	{"communication", required_argument, 0, 'c'},
	{"flowcontrol", required_argument, 0, 'f'},
	{"verify", no_argument, 0, 'v'},
	{"output", no_argument, 0, 'o'},
	{"debug", no_argument, 0, 'd'},
	{"active", no_argument, 0, 'a'},
	{"passive", no_argument, 0, 'p'},
	{"loopback", no_argument, 0, 'l'},
	{"mode", no_argument, 0, 'm'},
	{"pause", no_argument, 0, 'z'},
	{"packet", no_argument, 0, 'k'},
	{"stop", no_argument, 0, 't'},
	{"daemon", no_argument, 0, 'i'},
	{"time",required_argument,0,'h'},
	{0, 0, 0, 0}
};

static struct FC_DATA {
	char *fc_string;
	enum flowcontrol fc;
} flowcontrolopt_arr[] = {
	{"NONE", FC_NONE},
	{"none", FC_NONE},
	{"xonxoff", FC_XONXOFF},
	{"XONXOFF", FC_XONXOFF},
	{"dtrdsr", FC_DTRDSR},
	{"DTRDSR", FC_DTRDSR},
	{"rtscts", FC_RTSCTS},
	{"RTSCTS", FC_RTSCTS},
	{"both", FC_BOTH},
	{"BOTH", FC_BOTH},
	{0, FC_NONE}
};

int main(int argc, char *argv[], char *envp[])
{
	pthread_t SendThreadHandle, RecvThreadHandle,
		ShowThreadHandle, LoopThreadHandle,
		RS485ThreadHandle;
	int CreateThreadResult = -1;
	int PortHandle = -1;
	speed_t speed;
	struct termios PortTermios;
	struct serial_struct serinfo;
	pthread_attr_t ThreadAttr;
	int i;
	int exitcount = 0;
	struct sched_param ThreadParam;
	int option_index = 0;
	int c;
	int f_daemon = 0;
	unsigned char tcr, cpr;
	unsigned short dlldlm;

	ProgName = argv[0];
	if(argc == 1)
		usage();

	// setup send data buffer
	basechar = '0';
	stepcount = 64;
	for(i = 0; i < MAX_DATA_LEN; i++)
		WriteData[i] = basechar + (i % stepcount);

	while(1) {
		c = getopt_long (argc, argv,
			"tTiIsSrRaApPb:B:c:C:f:F:vVoOdDlLm:M:z:Z:k:K:h:",
            long_options, &option_index);
		/* Detect the end of the options. */
        if (c == -1)
        	break;

		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;
			case 't':
			case 'T':
				if (optarg)
					printf("option i sould not have extra parameter\n");
				f_stopOnError = 1;
				break;
			case 'i':
			case 'I':
				if (optarg)
					printf("option i sould not have extra parameter\n");
				f_daemon = 1;
				break;
			case 's':
			case 'S':
				if (optarg)
					printf("option s sould not have extra parameter\n");
				f_send = 1;
				break;
			case 'r':
			case 'R':
				if (optarg)
					printf("option r sould not have extra parameter\n");
				f_recv = 1;
				break;
			case 'm':
			case 'M':
				if(strcmp(optarg, "232") == 0)
					f_mode = MODE_RS232;
				else if(strcmp(optarg, "422") == 0)
					f_mode = MODE_RS422;
				else if(strcmp(optarg, "485") == 0)
					f_mode = MODE_RS485;
				else {
					printf("operation mode invalid\n");
					return -1;
				}
				break;
			case 'a':
			case 'A':
				if (optarg)
					printf("option a sould not have extra parameter\n");
				f_interaction = IA_ACTIVE;
				break;
			case 'p':
			case 'P':
				if (optarg)
					printf("option p sould not have extra parameter\n");
				f_interaction = IA_PASSIVE;
				break;
			case 'l':
			case 'L':
				if (optarg)
					printf("option l sould not have extra parameter\n");
				f_interaction = IA_LOOP;
				break;
			case 'b':
			case 'B':
				if(!optarg)
					printf("baudrate parameter invalid\n");
				baudrate = atoi(optarg);
				//jinxin comment to support high baudrate				
				/*
				if(baudrate > 921600 || baudrate < 50) {
					printf("baudrate value should be from 50 to 921600\n");
					return -1;
				}
				*/
				break;
			case 'h':
			case 'H':
				if(!optarg)
					printf("Time parameter invalid\n");
				stop_time = atoi(optarg);
				break;
				
			case 'c':
			case 'C':
				if(!optarg)
					printf("communication parameter invalid\n");
				switch(*optarg) {
					case 'n':
					case 'N':
						/* no parity*/
						f_parity = PARITY_NONE;
						break;
					case 'e':
					case 'E':
						/* even parity*/
						f_parity = PARITY_EVEN;
						break;
					case 'o':
					case 'O':
						/* odd parity*/
						f_parity = PARITY_ODD;
						break;
					case 'm':
					case 'M':
						/* mark parity*/
						f_parity = PARITY_MARK;
						break;
					case 's':
					case 'S':
						/* space parity*/
						f_parity = PARITY_SPACE;
						break;
					default:
						printf("communication parity parameter invalid\n");
						return -1;
						break;
				}
				databit = (*(optarg + 1)) - '0';
				if(databit > 8 || databit < 5)
					printf("communication databit parameter invalid\n");
				stopbit = (*(optarg + 2)) - '0';
				if(stopbit > 2 || stopbit < 1)
					printf("communication stopbit parameter invalid\n");
				break;
			case 'f':
			case 'F':
				if(!optarg)
					printf("flow control parameter invalid\n");
				i = 0;
				while(flowcontrolopt_arr[i].fc_string != 0) {
					if(strcmp(flowcontrolopt_arr[i].fc_string, optarg) == 0) {
						f_flowcontrol = flowcontrolopt_arr[i].fc;
						break;
					}
					i++;
				}
				if(!flowcontrolopt_arr[i].fc_string) {
					printf("flow control parameter invalid\n");
					return -1;
				}
				break;
			case 'k':
			case 'K':
				if(!optarg)
					printf("packet size parameter invalid\n");
				if(atoi(optarg) > 0 && atoi(optarg) <= MAX_DATA_LEN) {
					packet_size = atoi(optarg);
				} else
					printf("packet size parameter invalid (1~%d)\n",
						MAX_DATA_LEN);
				break;
			case 'z':
			case 'Z':
				if(!optarg)
					printf("packet pause period parameter invalid\n");
				if(atoi(optarg) >= 0) {
					pause_period = atoi(optarg);
				} else
					printf("packet pause period parameter invalid\n");
				break;
			case 'v':
			case 'V':
				if (optarg)
					printf("option v sould not have extra parameter\n");
				f_verify = 1;
				break;
			case 'o':
			case 'O':
				if (optarg)
					printf("option o sould not have extra parameter\n");
				f_output = 1;
				break;
			case 'd':
			case 'D':
				if (optarg)
					printf("option d sould not have extra parameter\n");
				f_debug = 1;
				break;
			case '?':
				/* getopt_long already printed an error message. */
				break;
			default:
				printf("invalid parameters\n");
				usage();
				return -2;
				break;
		}
	}

	printf("Program will\n");
	printf("using %s as testing serial port\n", argv[optind]);
	if(f_send) {
		printf("send specific pattern date to testing serial port\n");
		printf("sending length will be %d bytes per packet\n", packet_size);
		printf("sending will pause %d ms per packet\n", pause_period);
	}
	if(f_recv) {
		printf("receive date from testing serial port\n");
		if(f_verify)
			printf("verify the received data\n");
		else
			printf("not verify the received data\n");
	}
	switch(f_interaction) {
		case IA_ACTIVE:
			printf("start transmit immediately\n");
			break;
		case IA_PASSIVE:
			printf("wait for first character from active side\n");
			break;
		case IA_LOOP:
			printf("act as a loop-back connector\n");
			break;
		default:
			break;
	}

	if(f_output)
		printf("update statistic data during testing\n");
	else
		printf("show statistic data after terminated\n");

	if(f_debug)
		printf("print some debug message\n");

	printf("using baudrate %d bits/sec\n", baudrate);
	PortHandle = open((char *)argv[optind], O_RDWR | O_NOCTTY | O_NDELAY);
	if(PortHandle == -1) {
		printf("open_port: Unable to open %s\n", (char *)argv[optind]);
		return -1;
	}
	printf("Run time is:%d\n",stop_time);
	debug_printf("PortHandle:%d\n", PortHandle);

	tcflush(PortHandle, TCIOFLUSH);
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
		printf("using no parity\n");
		break;
	case PARITY_EVEN:
		printf("using even parity\n");
		PortTermios.c_cflag |= PARENB;
		PortTermios.c_iflag |= INPCK;      /* enable input parity checking */
		break;
	case PARITY_ODD:
		printf("using odd parity\n");
		PortTermios.c_cflag |= PARENB | PARODD;
		PortTermios.c_iflag |= INPCK;      /* enable input parity checking */
 		break;
	case PARITY_MARK:
		printf("using mark parity\n");
		PortTermios.c_cflag |= PARENB | CMSPAR | PARODD;
		PortTermios.c_iflag |= INPCK;      /* enable input parity checking */
		break;
	case PARITY_SPACE:
		printf("using space parity\n");
		PortTermios.c_cflag |= PARENB | CMSPAR;
		PortTermios.c_iflag |= INPCK;      /* enable input parity checking */
		break;
	default:
		goto ERROR_END;
		break;
	}

	PortTermios.c_cflag &= ~CSIZE;
	switch(databit) {
	case 5:
		printf("using 5 databit\n");
		PortTermios.c_cflag |= CS5;
		databit = 5;
		datamask = 0x1F;
		break;
	case 6:
		printf("using 6 databit\n");
		PortTermios.c_cflag |= CS6;
		databit = 6;
		datamask = 0x3F;
		break;
	case 7:
		printf("using 7 databit\n");
		PortTermios.c_cflag |= CS7;
		databit = 7;
		datamask = 0x7F;
		break;
	case 8:
		printf("using 8 databit\n");
		PortTermios.c_cflag |= CS8;
		databit = 8;
		datamask = 0xFF;
		break;
	default:
		goto ERROR_END;
		break;
	}

	switch(stopbit) {
	case 1:
		printf("using 1 stopbit\n");
		PortTermios.c_cflag &= ~CSTOPB;
		break;
	case 2:
		if(databit == 5)
			printf("using 1.5 stopbit\n");
		else
			printf("using 2 stopbit\n");
		PortTermios.c_cflag |= CSTOPB;
		break;
	default:
		goto ERROR_END;
		break;
	}

	switch(f_flowcontrol) {
	case FC_NONE:
		printf("using no flow control\n");
		PortTermios.c_cflag &= ~CRTSCTS;
		PortTermios.c_cflag &= ~CDTRDSR;
		PortTermios.c_iflag &= ~(IXON | IXOFF);      /* input/output flow */
		PortTermios.c_iflag &= ~IXANY;
		break;
	case FC_XONXOFF:
		printf("using XOn/XOff software flow control\n");
		PortTermios.c_cflag &= ~CRTSCTS;
		PortTermios.c_cflag &= ~CDTRDSR;
		PortTermios.c_iflag |= (IXON | IXOFF);      /* input/output flow */
		PortTermios.c_iflag |= IXANY;
		break;
	case FC_RTSCTS:
		printf("using RTS/CTS hardware flow control\n");
		PortTermios.c_cflag |= CRTSCTS;
		PortTermios.c_cflag &= ~CDTRDSR;
		PortTermios.c_iflag &= ~(IXON | IXOFF);     /* no sw flow ctrl */
		PortTermios.c_iflag |= IXANY;
		break;
	case FC_DTRDSR:
		printf("using DTR/DSR hardware flow control\n");
		PortTermios.c_cflag |= CDTRDSR;
		PortTermios.c_cflag &= ~CRTSCTS;
		PortTermios.c_iflag &= ~(IXON | IXOFF);     /* no sw flow ctrl */
		PortTermios.c_iflag |= IXANY;
		break;
	case FC_BOTH:
		printf("using XOn/XOff and RTS/CTS flow control\n");
		PortTermios.c_cflag |= CRTSCTS;
		PortTermios.c_iflag |= (IXON | IXOFF);      /* input/output flow */
		PortTermios.c_iflag |= IXANY;
		break;
	default:
		goto ERROR_END;
		break;
	}

	switch(f_mode) {
	case MODE_RS232:
		printf("using RS-232 transmittion mode\n");
		PortTermios.c_iflag &= ~IRS422;
		PortTermios.c_iflag &= ~IRS485;
		break;
	case MODE_RS422:
		printf("using RS-422 transmittion mode\n");
		PortTermios.c_iflag |= IRS422;
		PortTermios.c_iflag &= ~IRS485;
		break;
	case MODE_RS485:
		printf("using RS-485 transmittion mode\n");
		PortTermios.c_iflag &= ~IRS422;
		PortTermios.c_iflag |= IRS485;
		break;
	default:
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

	tcsetattr(PortHandle, TCSANOW, &PortTermios);
	if(f_debug)
		printTermios(&PortTermios);

#ifdef	CONFIG_NOT_STD_BANDRATE
	serinfo.reserved_char[0] = 0;
	if (ioctl(PortHandle, TIOCGSERIAL, &serinfo) < 0) {
		printf("Cannot get serial info");
		goto ERROR_END;
	}

	serinfo.flags |= ASYNC_SPD_CUST;
	if(serinfo.baud_base > 921600){
		//for PCIe958, serinfo.baud_base = 3906250( 62500000/16)
		switch(baudrate){
			case 1200:
			case 2400:
			case 4800:
			case 9600:
			case 19200:
			case 38400:
			case 57600:
			case 115200:
			case 230400:
				tcr = 4;
				cpr = 32;
				dlldlm = (int)((double)(serinfo.baud_base*16)/(tcr*cpr*baudrate/8)+0.5);
				serinfo.custom_divisor = (tcr << 24) + (cpr << 16) + dlldlm;
				break;
			case 460800:
			case 921600:
				tcr = 4;
				cpr = 8;
				dlldlm = (int)((double)(serinfo.baud_base*16)/(tcr*cpr*baudrate/8)+0.5);
				serinfo.custom_divisor = (tcr << 24) + (cpr << 16) + dlldlm;
				break;
			case 1843200:
			case 3686400:
				tcr = 4;
				cpr = 34;
				dlldlm = (int)((double)(serinfo.baud_base*16)/(tcr*baudrate*cpr/8)+0.5);
				serinfo.custom_divisor = (tcr << 24) + (cpr << 16) + dlldlm;
				break;
			case 7812500:
				tcr = 4;
				cpr = 8;
				dlldlm = (int)((double)(serinfo.baud_base*16)/(tcr*baudrate*cpr/8)+0.5);
				serinfo.custom_divisor = (tcr << 24) + (cpr << 16) + dlldlm;
				break;
			case 8928571:
				tcr = 7;
				cpr = 8;
				dlldlm = (int)((double)(serinfo.baud_base*16)/(tcr*baudrate*cpr/8)+0.5);
				serinfo.custom_divisor = (tcr << 24) + (cpr << 16) + dlldlm;
				break;
			case 10416666:
				tcr = 6;
				cpr = 8;
				dlldlm = (int)((double)(serinfo.baud_base*16)/(tcr*baudrate*cpr/8)+0.5);
				serinfo.custom_divisor = (tcr << 24) + (cpr << 16) + dlldlm;
				break;
			case 12500000:
				tcr = 5;
				cpr = 8;
				dlldlm = (int)((double)(serinfo.baud_base*16)/(tcr*baudrate*cpr/8)+0.5);
				serinfo.custom_divisor = (tcr << 24) + (cpr << 16) + dlldlm;
				break;
			case 15625000:
				tcr = 4;
				cpr = 8;
				dlldlm = (int)((double)(serinfo.baud_base*16)/(tcr*baudrate*cpr/8)+0.5);
				serinfo.custom_divisor = (tcr << 24) + (cpr << 16) + dlldlm;
				break;
			default:
				if(baudrate > 15625000) {
					printf("baudrate value is out of range,it should be  within 15625000\n");
					return -1;
				}
				break;
		}
	}
	else{	//for case: serinfo.baud_base = 921600
		if(baudrate > 921600 || baudrate < 50) {
			printf("baudrate value should be from 50 to 921600\n");
			return -1;
		}
		serinfo.custom_divisor = serinfo.baud_base / baudrate;

	}
	//printf("custom_divisor=%x\n", serinfo.custom_divisor);
	debug_printf("baudrate %d divisor will be %d\n",
		baudrate, serinfo.custom_divisor);

	if (ioctl(PortHandle, TIOCSSERIAL, &serinfo) < 0) {
		printf("Cannot set serial info");
		goto ERROR_END;
	}
#else
	memset(&PortTermios, 0, sizeof(struct termios));
	/* C_ISPEED     Input baud (new interface)
       C_OSPEED     Output baud (new interface)
    */
    switch (baudrate) {
    case 110:
        speed = B110;
        break;
    case 300:
        speed = B300;
        break;
    case 600:
        speed = B600;
        break;
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    case 230400:
        speed = B230400;
        break;
    case 460800:
        speed = B460800;
        break;
    case 500000:
        speed = B500000;
        break;
    case 576000:
        speed = B576000;
        break;
    case 921600:
        speed = B921600;
        break;
    case 1000000:
        speed = B1000000;
        break;
   case 1152000:
        speed = B1152000;
        break;
    case 1500000:
        speed = B1500000;
        break;
    case 2500000:
        speed = B2500000;
        break;
    case 3000000:
        speed = B3000000;
        break;
    case 3500000:
        speed = B3500000;
        break;
    case 4000000:
        speed = B4000000;
        break;
    default:
        speed = B9600;
       
		fprintf(stderr,
               "WARNING Unknown baud rate %d for %s (B9600 used)\n",
               baudrate, (char *)argv[optind]);
        
    }

	/* Set the baud rate */
    if ((cfsetispeed(&PortTermios, speed) < 0) ||
        (cfsetospeed(&PortTermios, speed) < 0)) {
        close(PortHandle);
        PortHandle = -1;
        return -1;
    }	

#endif
	pthread_attr_init(&ThreadAttr);
	pthread_attr_setstacksize(&ThreadAttr, 0x10000); // 4KB
	bRunning = 1;

	clock_gettime(CLOCK_REALTIME, &starttime);

	if(f_send || f_recv)
		printf("Press ENTER key to exit...\n");

	if(f_mode == MODE_RS485 && f_send && f_recv) {
		if(f_interaction == IA_LOOP) {
			printf("RS-485 mode can't use loop-back test\n");
			goto ERROR_END;
		}
		ThreadParam.sched_priority = 1;
		pthread_attr_setschedparam(&ThreadAttr, &ThreadParam);
		CreateThreadResult = pthread_create(&RS485ThreadHandle,
			&ThreadAttr, &RS485Thread, (void *)PortHandle);
		if(CreateThreadResult != 0) {
			printf("Create Recv Thread Failed!\n");
			goto ERROR_END;
		}
		if(f_output) {
			ThreadParam.sched_priority = 5;
			pthread_attr_setschedparam(&ThreadAttr, &ThreadParam);
			CreateThreadResult = pthread_create(&ShowThreadHandle,
				&ThreadAttr, &ShowThread, (void *)PortHandle);
			if(CreateThreadResult != 0) {
				printf("Create Show Thread Failed!\n");
				goto ERROR_END;
			}
		}
		if(f_daemon) {
			goto DAEMON;
		}
		//getchar();
		StopTime();
		if(!f_output) {
			if (ioctl(PortHandle, TIOCGICOUNT, &icount) < 0) {
				printf("Cannot get interrupt counters");
				goto END;
			}
			clock_gettime(CLOCK_REALTIME, &nowtime);
			printf(	"R %u, %u BPS<= (%02u:%02u:%02u) "
					"=>S %u, %u BPS OE:%u, %d%5s\n",
				RecvTotal, RecvThroughput,
				(nowtime.tv_sec - starttime.tv_sec) / (60 * 60),
				((nowtime.tv_sec - starttime.tv_sec) % (60 * 60)) / 60,
				(nowtime.tv_sec - starttime.tv_sec) % 60,
				SendTotal, SendThroughput,
				icount.overrun,
				timeoutcount,
				" ");
			fflush(stdout);
		}
	} else {
		if(f_interaction != IA_LOOP) {
			if(f_recv) {
				ThreadParam.sched_priority = 1;
				pthread_attr_setschedparam(&ThreadAttr, &ThreadParam);
				CreateThreadResult = pthread_create(&RecvThreadHandle,
					&ThreadAttr, &RecvThread, (void *)PortHandle);
				if(CreateThreadResult != 0) {
					printf("Create Recv Thread Failed!\n");
					goto ERROR_END;
				}
			}

			if(f_send) {
				ThreadParam.sched_priority = 5;
				pthread_attr_setschedparam(&ThreadAttr, &ThreadParam);
				CreateThreadResult = pthread_create(&SendThreadHandle,
					&ThreadAttr, &SendThread, (void *)PortHandle);
				if(CreateThreadResult != 0) {
					printf("Create Send Thread Failed!\n");
					goto ERROR_END;
				}
			}

			if((f_send || f_recv) && f_output) {
				ThreadParam.sched_priority = 5;
				pthread_attr_setschedparam(&ThreadAttr, &ThreadParam);
				CreateThreadResult = pthread_create(&ShowThreadHandle,
					&ThreadAttr, &ShowThread, (void *)PortHandle);
				if(CreateThreadResult != 0) {
					printf("Create Show Thread Failed!\n");
					goto ERROR_END;
				}
			}
			if(f_daemon) {
				goto DAEMON;
			}
			if(f_send || f_recv) {
				//getchar();
				StopTime();
			}

			if(!f_output) {
				if (ioctl(PortHandle, TIOCGICOUNT, &icount) < 0) {
					printf("Cannot get interrupt counters");
					goto END;
				}
				clock_gettime(CLOCK_REALTIME, &nowtime);
				printf("R %llu, %llu BPS<= (%02u:%02u:%02u) =>S %llu, %llu BPS OE:%u, %d%5s\n",
					RecvTotal, RecvThroughput,
					(nowtime.tv_sec - starttime.tv_sec) / (60 * 60),
					((nowtime.tv_sec - starttime.tv_sec) % (60 * 60)) / 60,
					(nowtime.tv_sec - starttime.tv_sec) % 60,
					SendTotal, SendThroughput,
					icount.overrun,
					timeoutcount,
					" ");
				fflush(stdout);
			}
		}
		else
		{
			ThreadParam.sched_priority = 1;
			pthread_attr_setschedparam(&ThreadAttr, &ThreadParam);
			CreateThreadResult = pthread_create(&LoopThreadHandle,
				&ThreadAttr, &LoopThread, (void *)PortHandle);
			if(CreateThreadResult != 0) {
				printf("Create Loop Thread Failed!\n");
				goto ERROR_END;
			}
			if(f_daemon) {
				goto DAEMON;
			}
			//getchar();
			StopTime();
		}
	}

	bRunning = 0;
END:
	while(f_interaction != IA_LOOP) {
		if(f_send || f_recv || f_output) {
			usleep(10000); // wait until threads terminated.
			exitcount++;
			if(exitcount > 100)
				break;  // to avoid dead lock
		} else
			break;
	}
	if (ioctl(PortHandle, TIOCGICOUNT, &icount) < 0) {
		printf("Cannot get interrupt counters");
		goto ERROR_END;
	}
	printf("total rx:%u\n", icount.rx);
	printf("total tx:%u\n", icount.tx);
	printf("frame error:%u\n", icount.frame);
	printf("parity error:%u\n", icount.parity);
	printf("break error:%u\n", icount.brk);
	printf("overrun:%u\n", icount.overrun);

ERROR_END:
	close(PortHandle);
	printf("Program Terminated!\n");
	return 0;
	
DAEMON:
	while(1) {
		sleep(100);
	}
	return 0;
}
