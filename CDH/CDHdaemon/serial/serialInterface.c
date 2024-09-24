
/*
	Serial interface functions, here the I/O functions wrappers
	towards the serial line will be defined to be used with
	the simpleDataLink library.
	
	Two lines will be defined, one is a loopback line which uses
	a circular buffer to simulate a serial line closed on itself and which
	can be used for testing purposes.
	The other is the actual line using the UART peripheral
*/

#include "bufferUtils.h"
#include "simpleDataLink.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

//get maximum payload length
uint32_t getMaxLen(){
	return SDL_MAX_PAY_LEN;
}

// LOOPBACK LINE ------------------------------
// buffer declaration for loopback line
circular_buffer_handle buff;
uint8_t buffArray[2048];

//defining txFunc and rxFunc for loopback line
uint8_t txFuncLoopback(uint8_t byte){
	if(!cBuffPushToFill(&buff,&byte,1,1)) return 0;
	printf("%02x ",byte);
	return 1;
}
uint8_t rxFuncLoopback(uint8_t* byte){
	if(!cBuffPull(&buff,byte,1,0)) return 0;
	printf("%02x ",*byte);
	return 1;
}

//serial line handle struct
serial_line_handle loopbackLine;

//flag to store if loopback line was initialized
uint8_t loopbackInit=0;

void initLoopback(){
	//init buffer
	cBuffInit(&buff,buffArray,sizeof(buffArray),0);
	sdlInitLine(&loopbackLine,&txFuncLoopback,&rxFuncLoopback,0,0);
	loopbackInit=1;
	printf("Loopback Line initialized\n");
}

uint8_t sendLoopback(uint8_t* buff, uint32_t len){
	if(!loopbackInit){
		printf("ERROR! initialize loopback line with initLoopback() before use\n");
		return 0;
	}
	printf("TX loopback: ");
	uint8_t retVal=sdlSend(&loopbackLine,buff,len,0);
	printf("\n");
	return retVal;
}

uint32_t receiveLoopback(uint8_t* buff, uint32_t len){
	if(!loopbackInit){
		printf("ERROR! initialize loopback line with initLoopback() before use\n");
		return 0;
	}
	printf("RX loopback: ");
	uint8_t retVal=sdlReceive(&loopbackLine,buff,len);
	printf("\n");
	return retVal;
}

//UART line -----------------------------------

#define UART_DEV "/dev/serial0" //device name

//store that uart line was initialized
uint8_t uartInit=0;
int uartfd; //UART file descriptor
serial_line_handle uartLine; //uart line handle

//defining txFunc and rxFunc for uart line
uint8_t txFuncUart(uint8_t byte){
	//temporarily set descriptor as blocking
	int state=fcntl(uartfd,F_GETFL);
	fcntl(uartfd,F_SETFL,state & ~O_NONBLOCK);
	if(write(uartfd,&byte,1) !=1) return 0;
	//return to blocking
	fcntl(uartfd,F_SETFL,state);
	return 1;
}
uint8_t rxFuncUart(uint8_t* byte){
	if(read(uartfd,byte,1)!=1){//printf("%02x 0 \n", *byte);
	 return 0;}
	//printf("%02x 1\n", *byte);
	return 1;
}

//defining simpleDalaLink sdlTimeTick function
uint32_t sdlTimeTick(){
	return (uint32_t) clock();
}

//init function, the timeout (in python format) and number of retries should be passed
void initUART(float timeout, uint8_t retries){
	uartfd = open(UART_DEV, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if(uartfd == -1){
		printf("ERROR Failed to open %s\n",UART_DEV);
		return;
	}
	
	if(!isatty(uartfd)){
		printf("ERROR, %s is not a tty device\n",UART_DEV);
		close(uartfd);
		return;
	}
	
	struct termios config;
	
	if(tcgetattr(uartfd, &config) < 0){
		printf("ERROR, cannot get %s configuration\n",UART_DEV);
		close(uartfd);
		return;
	}
	
	//setting input flags
	//flags disabled
	config.c_iflag &= ~(IGNBRK | PARMRK | INPCK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF);
	//flags enabled
	config.c_iflag |= (IGNPAR);
	
	//setting output flags
	//flags disabled
	config.c_oflag &= ~(OPOST | ONLCR | OCRNL | ONLRET | OFILL | OFDEL);
	//flags enabled
	config.c_oflag |= (ONOCR);
	
	//setting control flags
	//flags disabled
	config.c_cflag &= ~(CSIZE | CSTOPB | PARENB |HUPCL);
	//flags enabled
	config.c_cflag |= (CS8 | CREAD | CLOCAL);
	
	//setting local flags
	//flags disabled
	config.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL | IEXTEN);
	//flags enabled
	config.c_lflag |= (NOFLSH | TOSTOP);
	
	//setting cc array
	config.c_cc[VTIME]=0;
	config.c_cc[VMIN]=1;
	
	//setting baud rate
	if(cfsetispeed(&config, B115200) < 0 || cfsetospeed(&config, B115200) < 0){
		printf("ERROR, cannot set %s baud rate\n",UART_DEV);
		close(uartfd);
		return;
	}
	
	//apply configuration
	if(tcsetattr(uartfd, TCSAFLUSH, &config) < 0){
		printf("ERROR, cannot set %s configuration\n",UART_DEV);
		close(uartfd);
		return;
	}
	
	//computing the timeout
	uint32_t intTimeout=(uint32_t)(timeout*CLOCKS_PER_SEC);
	
	//initializing serial line handle
	sdlInitLine(&uartLine,&txFuncUart,&rxFuncUart,intTimeout,retries);
	
	//signal that UART was correctly initialized
	//printf("%s correctly initialized\n",UART_DEV);
	
	uartInit=1;
	return;
}

void deinitUART(){
	close(uartfd);
	//printf("%s correctly de-initialized\n",UART_DEV);
	uartInit=0;
	return;
	
}

uint8_t sendUART(uint8_t* buff, uint32_t len, uint8_t ackWanted){
	if(!uartInit){
		printf("ERROR! initialize uart line with initUART() before use\n");
		return 0;
	}
	uint8_t retVal=sdlSend(&uartLine,buff,len, ackWanted);
	return retVal;
}

uint32_t receiveUART(uint8_t* buff, uint32_t len){
	if(!uartInit){
		printf("ERROR! initialize uart line with initUART() before use\n");
		return 0;
	}
	uint8_t retVal=sdlReceive(&uartLine,buff,len);
	//printf("%f retVal --- \n", retVal);
	return retVal;
}


