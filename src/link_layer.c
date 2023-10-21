// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
#define BUF_SIZE 256 // NOT SURE YET
/*struct termios oldtio_t; // I want to try to declare it outside the function just because we need to restore it but the application layer cannot see it
struct termios oldtio_r;*/

int alarmEnabled = FALSE;
int alarmCount = 0;
volatile int STOP = FALSE;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int checkbyte(char c, int count, LinkLayerRole role){
	switch(role){
		case Lltx:
			switch(count){
					case 0:
						if(c == 0x7E)
							return 1;
						else 
							return 0;
					case 1:
						if (c == 0x01)
							return 2;
						else if (c == 0x7E)
							return 1;
						else return 0;
					case 2:
						if (c == 0x07)
							return 3;
						else if (c == 0x7E)
							return 1;
						else return 0;
					case 3:
						if (c == 0x01 ^ 0x07)
							return 4;
						else if (c == 0x7E)
							return 1;
						else return 0;
					case 4:
						if (c == 0x7E)
							return 5;
						else return 0;
					default:
						return 0;				
				}
				break;
		
		case LlRx:
			switch(count){
				case 0:
					if(c == 0x7E)
						return 1;
					else 
						return 0;
				case 1:
					if (c == 0x03)
						return 2;
					else if (c == 0x7E)
						return 1;
					else return 0;
				case 2:
					if (c == 0x03)
						return 3;
					else if (c == 0x7E)
						return 1;
					else return 0;
				case 3:
					if (c == 0x03 ^ 0x03)
						return 4;
					else if (c == 0x7E)
						return 1;
					else return 0;
				case 4:
					if (c == 0x7E)
						return 5;
					else return 0;
				default:
					return 0;				
			}
			break;
		
		default:
			return 0;
	}
	
}

int sendSFrame(int fd, LinkLayerRole role){
	unsigned char buf[5];
	switch(role){
		case Lltx:
			buf[1] = 0x03;
			buf[2] = 0x03;
			break;
		case LlRx:
			buf[1] = 0x01;
			buf[2] = 0x07;
			break;
	}
	buf[0] = 0x7E;
	buf[3] = buf[1] ^ buf[2];
	buf[4] = buf[0];

	return write(fd, buf, 5);
}

int llopen(LinkLayer connectionParameters)
{
    int fd = setconnection(connectionParameters.serialPort);
	if (fd < 0) 
	{
		printf("Error in the connection\n");
		return -1;
	}
	int bytes;
	int count = 0;
	unsigned char buf[BUF_SIZE] = {0};
	switch(connectionParameters.role){
		
		case LlTx:
			(void)signal(SIGALRM, alarmHandler);			
			while(alarmCount < connectionParameters.nRetrasmissions && count < 5){
				while(bytes = read(fd, buf, 1) > 0 && count < 5){
					count = checkbyte(buf[0], count);     
				}
				/*bytes = read(fd, buf, BUF_SIZE);
				if (bytes == 5 && buf[1] ^ buf[2] == buf[3] && buf[0] == 0x7E && buf[4] == buf[0] && buf[1] == 0x01 && buf[2] == 0x07){
					printf("Everything cool\n");
					alarm(0);
					alarmEnabled = TRUE;
					alarmCount = 3;
				}*/
				if (alarmEnabled == FALSE && count < 5){
					sendSFrame(fd, LlTx);

					// Wait until all bytes have been written to the serial port
					sleep(1); // NOT SURE YET
					alarm(connectionParameters.timeout);
					alarmEnabled = TRUE;
					
				}
			
			}
			sleep(1);
			while(count < 5 && bytes = read(fd, buf, 1) > 0){
					count = checkbyte(buf[0], count);     
				}
			if (count != 5)
				return -1;
			break;
			
		case LlRx:
			while(count < 5){
				if(bytes = read(fd, buf, BUF_SIZE) > 0)
					count = checkbyte(buf[0], count);      
				//sleep(1);
			}
			
			
			if (sendSFrame(fd, LlRx) != 5)
				return -1;
		
			break;
			
		default:
			return -1;
	}
	

    return fd;
}

int setconnection(char *serialPort, LinkLayerRole role){
	int fd = open(serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }
	/*struct termios oldtio;*/
    struct termios newtio;
	// Save current port settings
	/*switch(role){
		case LlTx:
			if (tcgetattr(fd, &oldtio_t) == -1)
			{
				perror("tcgetattr");
				return -1;
			}
			break;
		case LlRx:
			if (tcgetattr(fd, &oldtio_r) == -1)
			{
				perror("tcgetattr");
				return -1;
			}
			break;
	}*/

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Read what is on the buffer right now, without waiting
	// Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    printf("New termios structure set\n");
	return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO
	
    // Restore the old port settings
    /*if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }*/

    if(close(fd) < 0)
		return -1;
	return 1;
}
