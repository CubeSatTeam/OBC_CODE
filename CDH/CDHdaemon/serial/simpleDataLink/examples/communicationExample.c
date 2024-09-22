/**
 * @file communicationExample.c
 * @author Simone Bollattino (simone.bollattino@gmail.com)
 * @brief Example with the simpleDataLink.h/.c library
 * 
 * In this example we will simulate an exchange of frames between two
 * nodes, the serial line will be simulated by two circular buffers, like
 * this:
 *  ____________                              ____________
 * |            |----------TxBuff----------->|            |
 * |   NODE 1   |                            |   NODE 2   |
 * |            |<---------RxBuff------------|            |
 * |____________|                            |____________|
 * 
 * Various tests will be executed:
 * Test 1 - Simple exchange without acks
 * Test 2 - line 2 sends without ack, line 1 sends with ack
 * Test 3 - Line 1 sends with ack, line 2 doesn't respond the first time
 * 			Line 2 sends with ack, now line 1 responds thr first time but the ack gets lost
 * Test 4 - Here we volountarily trigger a possible deadlock condition (both line1 and line2
 * 			waiting for the ack) to test the anti-lock feature, sadly we cannot completely
 * 			resolve the deadlock since we are simulating the response of line2 inside the 
 * 			callback of line 1 (so we cannot simulate contemporary line1 to respond to line2)
 * 			this should require a true multi-thread test but for us it's enough to see that 
 * 			it works in one direction (line2 will reach the timeout)
 * 
 */

#include "bufferUtils.h"
#include "simpleDataLink.h"
#include <stdio.h>

// buffers declaration
//Tx buffer, this simulates the serial TX line
circular_buffer_handle TxBuff;
uint8_t TxBuffArray[50];
//Rx buffer, this represents the serial RX line
circular_buffer_handle RxBuff;
uint8_t RxBuffArray[50];

//defining txFunc and rxFunc for both nodes
uint8_t txFunc1(uint8_t byte){
	if(!cBuffPushToFill(&TxBuff,&byte,1,1)) return 0;
	return 1;
}
uint8_t rxFunc1(uint8_t* byte){
	if(!cBuffPull(&RxBuff,byte,1,0)) return 0;
	return 1;
}
uint8_t txFunc2(uint8_t byte){
	if(!cBuffPushToFill(&RxBuff,&byte,1,1)) return 0;
	return 1;
}
uint8_t rxFunc2(uint8_t* byte){
	if(!cBuffPull(&TxBuff,byte,1,0)) return 0;
	return 1;
}

//serial line handle structs for both nodes
serial_line_handle line1;
serial_line_handle line2;
serial_line_handle injLine; //injecting line

//defining simpleDataLink sdlTimeTick function
uint32_t tickTime=0;
uint32_t sdlTimeTick(){
	return tickTime++;
}

//test payloads
char pay1[]="Hello ";
char pay2[]="World!";
char dummy[]="dummy";

uint8_t testNum=1;
char rxPay[20];
uint8_t retryNum=0;
//sdl test send callback definition
void __sdlTestSendCallback(serial_line_handle* line){
	if(testNum==2){
		if(line==&line1){
			printf("Line 2, received (%u): %s\n",sdlReceive(&line2,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);
		}
	}if(testNum==3){
		if(line==&line1){
			if(!retryNum){
				printf("Line 2, doesn't receive the first time\n");
				retryNum=1;
			}else{
				printf("Line 2, receives the second time\n");
				printf("Line 2, received (%u): %s\n",sdlReceive(&line2,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);
				retryNum=0;
			}
		}else{
			if(!retryNum){
				printf("Line 1, received (%u): %s\n",sdlReceive(&line1,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);
				printf("Line 1, ...but the ack gets lost\n");
				cBuffFlush(&TxBuff);
				retryNum=1;
			}else{
				printf("Line 1, receives again, ignoring thanks to the hash\n");
				printf("Line 1, received (%u)\n",sdlReceive(&line1,(uint8_t*)rxPay,sizeof(rxPay)));
				
				retryNum=0;
			}
		}
	}if(testNum==4){
		if(line==&line1){
			printf("Line 2, start sending %s with ack\n",pay2);
			printf("Line 2, sending: %s returned: %u\n",pay2,sdlSend(&line2,(uint8_t*)pay2,sizeof(pay2),1));
		}else{
			printf("Line 1, here we should loop line1 sdlSend but we already are inside line1 callback, line2 will timeout\n");
		}
	}
}

int main(){
	//init buffers
	cBuffInit(&TxBuff,TxBuffArray,sizeof(TxBuffArray),0);
	cBuffInit(&RxBuff,RxBuffArray,sizeof(RxBuffArray),0);

	//init lines
	sdlInitLine(&line1,&txFunc1,&rxFunc1,0,1);
	sdlInitLine(&line2,&txFunc2,&rxFunc2,0,1);

	printf("\nTEST %u ------------\n",testNum);

	//node 1 sends both payloads
	printf("Line 1, sending: %s returned: %u\n",pay1,sdlSend(&line1,(uint8_t*)pay1,sizeof(pay1),0));
	printf("Line 1, sending: %s returned: %u\n",pay2,sdlSend(&line1,(uint8_t*)pay2,sizeof(pay2),0));

	//line 2 receives both payloads
	printf("Line 2, received (%u): %s\n",sdlReceive(&line2,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);
	printf("Line 2, received (%u): %s\n",sdlReceive(&line2,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);

	testNum++;
	printf("\nTEST %u ------------\n",testNum);

	//node 2 sends a payload without ack
	printf("Line 2, sending: %s returned: %u\n",dummy,sdlSend(&line2,(uint8_t*)dummy,sizeof(dummy),0));

	//node 1 sends a payload with ack
	printf("Line 1, start sending %s with ack\n",pay1);
	printf("Line 1, sending: %s returned: %u\n",pay1,sdlSend(&line1,(uint8_t*)pay1,sizeof(pay1),1));
	//(line 2 receives inside the callback)

	//node 1 receives the first payload from line 2
	printf("Line 1, received (%u): %s\n",sdlReceive(&line1,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);

	testNum++;
	printf("\nTEST %u ------------\n",testNum);

	//node 1 sends a payload with ack
	printf("Line 1, start sending %s with ack\n",pay1);
	printf("Line 1, sending: %s returned: %u\n",pay1,sdlSend(&line1,(uint8_t*)pay1,sizeof(pay1),1));
	//(node 2 doesn't receive the first time but only the second one inside the callback)

	//now the buffer is not empty because node 2 received the first try frame
	printf("Line2, Rx buffer still has a frame from the retry:\n");
	printf("Line2, Rx buffer: "); cBuffPrint(&line2.rxBuff,PRINTBUFF_HEX | PRINTBUFF_NOEMPTY);
	
	//line 2 should ignore the second try frame thanks to the hash (sending the ack anyway)
	printf("Line 2, receives a second time but ignores thanks to hash\n");
	printf("Line 2, received (%u)\n",sdlReceive(&line2,(uint8_t*)rxPay,sizeof(rxPay)));

	printf("Line2, now the buffer is empty!\n");
	printf("Line2, Rx buffer: "); cBuffPrint(&line2.rxBuff,PRINTBUFF_HEX | PRINTBUFF_NOEMPTY);

	printf("Line 2, start sending %s with ack\n",pay2);
	printf("Line 2, sending: %s returned: %u\n",pay2,sdlSend(&line2,(uint8_t*)pay2,sizeof(pay2),1));

	testNum++;
	printf("\nTEST %u ------------\n",testNum);

	sdlInitLine(&line1,&txFunc1,&rxFunc1,0,1);
	sdlInitLine(&line2,&txFunc2,&rxFunc2,0,1);

	//node 2 sends a payload without ack
	printf("Line 2, sending: %s returned: %u\n",dummy,sdlSend(&line2,(uint8_t*)dummy,sizeof(dummy),0));

	//node 1 sends a payload with ack
	printf("Line 1, start sending %s with ack\n",pay1);
	printf("Line 1, sending: %s returned: %u\n",pay1,sdlSend(&line1,(uint8_t*)pay1,sizeof(pay1),1));
	//(line 2 also sends payload with ack inside callback, we cannot simulate line1 so line2 will timeout
	// but we at least verify that the mechanism works in one direction)

	//now line1 should receive the dummy payload
	printf("Line 1, received (%u): %s\n",sdlReceive(&line1,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);
	//line2 timed out waiting ack, but the frame was sent anyway
	printf("Even if line2 timed out waiting for ack, the frame was sent:\n");
	printf("Line 1, received (%u): %s\n",sdlReceive(&line1,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);

	printf("Line 2, received (%u): %s\n",sdlReceive(&line2,(uint8_t*)rxPay,sizeof(rxPay)),rxPay);

	printf("BYE -----------\n");
}