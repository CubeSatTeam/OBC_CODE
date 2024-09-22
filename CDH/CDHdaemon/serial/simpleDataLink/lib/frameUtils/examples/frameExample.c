/**
 * @file frameExample.c
 * @author Simone Bollattino (simone.bollattino@gmail.com)
 * @brief Examples with the frameUtils.h/.c library
 * 
 * In this examples we will simulate an incoming serial stream and use the
 * functions of frameUtils to detect incoming frames
 * 
 */

#include "bufferUtils.h"
#include "frameUtils.h"
#include <stdio.h>

// just to be sure we enable buffUtils debug (if it was disabled on the header)
#define BUFFERUTILS_ENABLEPRINT

// buffers declaration
//Tx buffer, this simulates the serial line transmitter
circular_buffer_handle TxBuff;
//Rx buffer, this represents the serial line receiver
circular_buffer_handle RxBuff;
uint8_t RxData[9];
//Frame buffer, this represents the frame 
circular_buffer_handle FrameBuff;

// static functions for repetive code
static void initBuffers(uint8_t* txdata,uint32_t txlen){
	//transmitter initialization
	cBuffInit(&TxBuff,txdata,txlen,txlen);
	printf("\nTX BUFFER:\t"); cBuffPrint(&TxBuff,PRINTBUFF_NOEMPTY);
	//receiver initialization
	cBuffInit(&RxBuff,RxData,sizeof(RxData),0);
	printf("RX BUFFER:\t"); cBuffPrint(&RxBuff,0);
}
static void transmit(circular_buffer_handle* txbuff){
		printf("TRANSMITING...\n");
		while(!cBuffEmpty(txbuff) && !cBuffFull(&RxBuff)){
			uint8_t data;
			//pulling one byte from Tx head
			uint8_t tmpNum=cBuffPull(txbuff,&data,1,0);
			//pushing into Rx tail
			cBuffPush(&RxBuff,&data,tmpNum,1);
		}
		printf("TX BUFFER:\t"); cBuffPrint(txbuff,PRINTBUFF_NOEMPTY);
		printf("RX BUFFER:\t"); cBuffPrint(&RxBuff,0);
}

int main(){
	/*
		EXAMPLE 1
		In this first very simple example we simulate a transmission in which the frame 
		has the format: |16| x | x | x |26| 
		So head and tail are 16 an 26 respectively, there is byte stuffing so 16 and 26 
		cannot appear inside the frame data and the packets are of length exactly 3.

		The transmission is limited by the size of the receiver buffer so every time it
		transmit enough bytes to fill the Rx buffer and then searchFrameAdvance() is called
		until it can no longer find frames
	*/
	uint8_t TxData1[]={4,16,0,3,26,16,2,8,26,2,0,16,2,8,26};

	//This is the rule that applies to this research:
	uint8_t head1[]={16};
	uint8_t tail1[]={26};
	search_frame_rule rule1 = {
		.head=head1,
		.headLen=sizeof(head1),
		.tail=tail1,
		.tailLen=sizeof(tail1),
		.minLen=2,
		.maxLen=2,
		.policy=hard
	};

	printf("\n EXAMPLE 1 --------------------------------------------\n");

	initBuffers(TxData1,sizeof(TxData1));
	
	do{
		transmit(&TxBuff);

		printf("SEARCHING...\n");

		/*
		Since head and tail sequences are different and we are using hard policy, it's safe to use SHIFTOUT_FOUND
		to speed up the execution, there are other cases in which it's not safe to use this flag, as we will see in
		the following example.
		*/

		while(searchFrameAdvance(&RxBuff,&FrameBuff,&rule1,SHIFTOUT_FULL | SHIFTOUT_FOUND)){
				printf("FOUND FRM:\t"); cBuffPrint(&FrameBuff,PRINTBUFF_NOEMPTY | PRINTBUFF_NONEWLINE);
				cBuffPull(&FrameBuff,NULL,1,0); cBuffPull(&FrameBuff,NULL,1,1);
				printf("\tFRAME DATA:\t"); cBuffPrint(&FrameBuff,PRINTBUFF_NOEMPTY);
				printf("RX BUFFER:\t"); cBuffPrint(&RxBuff,0);
		}

		printf("NO MORE FRAMES FOUND\n");

	}while(!cBuffEmpty(&TxBuff));

	/*
	EXAMPLE 2
	This second example is a bit more complex, in this case we will have a frame with the same sequence
	for head and tail = 16, minimum length 3 and maximum length 5, we will also simulate a noisy line 
	in which some garbage bytes can be present between frames, as follows:
	... | D  | D  | 16 | n  | n  | 16 | D  | D  | D  | D  | 16 | n  | n  | n  | n  | 16 | D  | ...
	We will show that in this case it's not safe to use the SHIFTOUT_FOUND flag but instead it's better
	to use SHIFTOUT_NEXT coupled with a mechanism to check frame validity.

	The example of transmitted data is the following:
	*/
	uint8_t TxData2[]={4,2,16,0,0,0,16,2,5,3,16,0,0,16,8,2,16,0,16,4,2};
	/*
	In this case valid frames have the following format:
	|16|x|x|check|16| where check is a checksum that in this simple example corresponds to the number of
	frame bytes (head and tail excluded). The transmitter buffer was also filled with noise between frames
	(represented by 0s).
	*/

	//This is the rule that applies to this research:
	uint8_t head2[]={16};
	uint8_t tail2[]={16};
	search_frame_rule rule2 = {
		.head=head2,
		.headLen=sizeof(head2),
		.tail=tail2,
		.tailLen=sizeof(tail2),
		.minLen=2,
		.maxLen=5,
		.policy=hard
	};

	printf("\n EXAMPLE 2 --------------------------------------------\n");
	printf("\n PART 2.1 -------------------------\n");

	initBuffers(TxData2,sizeof(TxData2));

	do{
		transmit(&TxBuff);

		printf("SEARCHING...\n");

		/*
		In this first part of the example we use the SHIFTOUT_FOUND as in the previous one.
		We will see that due to the synchronization between the transmission and the searchFrameAdvance()
		function, only "ghost frames" made of noise will be found, that's because the SHIFTOUT_FOUND
		will remove frames from Rx buffer when found, effectively shifting out the head and tail sequences
		of the previous and next valid frames, which will never be found. 
		*/

		while(searchFrameAdvance(&RxBuff,&FrameBuff,&rule2,SHIFTOUT_FULL | SHIFTOUT_FOUND)){
				printf("FOUND FRM:\t"); cBuffPrint(&FrameBuff,PRINTBUFF_NOEMPTY | PRINTBUFF_NONEWLINE);
				cBuffPull(&FrameBuff,NULL,1,0); cBuffPull(&FrameBuff,NULL,1,1);
				printf("\tFRAME DATA:\t"); cBuffPrint(&FrameBuff,PRINTBUFF_NOEMPTY);
				printf("RX BUFFER:\t"); cBuffPrint(&RxBuff,0);
		}

		printf("NO MORE FRAMES FOUND\n");

	}while(!cBuffEmpty(&TxBuff));

	printf("\n PART 2.2 -------------------------\n");
	/*
	Now we will repeat the example by using the SHIFTOUT_NEXT flag instead, performing the frame check to discard
	ghost frames made by noise.
	*/

	initBuffers(TxData2,sizeof(TxData2));

	do{
		transmit(&TxBuff);

		printf("SEARCHING...\n");

		while(searchFrameAdvance(&RxBuff,&FrameBuff,&rule2,SHIFTOUT_FULL | SHIFTOUT_NEXT)){
				printf("FOUND FRM:\t"); cBuffPrint(&FrameBuff,PRINTBUFF_NOEMPTY | PRINTBUFF_NONEWLINE);
				cBuffPull(&FrameBuff,NULL,1,0); cBuffPull(&FrameBuff,NULL,1,1);
				printf("\tFRAME DATA:\t"); cBuffPrint(&FrameBuff,PRINTBUFF_NOEMPTY | PRINTBUFF_NONEWLINE);
				//frame validity check
				if(FrameBuff.elemNum == cBuffReadByte(&FrameBuff,1,0)){
					printf("VALID FRAME!\n");
				}else{
					printf("INVALID FRAME.\n");
				}

				printf("RX BUFFER:\t"); cBuffPrint(&RxBuff,0);
		}

		printf("NO MORE FRAMES FOUND\n");

	}while(!cBuffEmpty(&TxBuff));

	/*
	EXAMPLE 3
	In this third example we will show how to use the library to receive tail-less frames
	with variable length, the format is the following:
	|HEAD|LEN| D  | D  |  ...  | D  |CHECK|
	The frame starts with HEAD header (value 16), the second byte is a length byte which 
	represents the total number of data bytes D (must be at least 1), then the frame ends 
	with a checksum  (in this case it's the sum of all data bytes).
	The receiver is implemented as follows:
	- It searches for a frame starting with HEAD and of length 1 (so HEAD+LEN bytes), the
	  buffer is not shifted out after the frame is found.
	- When a frame is found, the LEN byte is read and the receiver searches for a frame 
	  starting with HEAD and of length LEN+2 (so accounting for LEN byte, data and CHECK 
	  byte)
	- When enough bytes are found, thechecksum is verified to approve the frame.
	To further complicate things, we cannot ensure that there are no head bytes (16) inside 
	the frame data, so we shall use the hard policy of searchFrameAdvance() and resort to 
	the checksum to verify a frame validity.
	There is another problem related to this approach, consider a situation in which the Rx
	buffer has this content:
	        RECEIVER STARTS HERE
			v
	|HEAD|4|x|x|16|40|CHECK|HEAD|4|x|x|x|x|CHECK|
	| VALID FRAME          | VALID FRAME        |
	           | GHOST FRAME ....................
	There's a 16 inside the first valid frame which will be interpreted as a ghost
	frame head sequence, there's also a high number after the 16, in this case the receiver
	would wait until enough frames arrive after the ghost one so that the ghost frame
	checksum position is reached, potentially hang for an indefinite time.
	There are a numbers of possibilities to account for this problem:
	- Immediately discard the frame if the LEN is too high to fit on the receiver buffer.
	- Completely shift out a frame which passed the checksum verification, similarly to 
	  what the SHIFTOUT_FOUND flag does, this would avoid finding ghost headers inside the
	  data of the frame but would not work in case for some reasons the checksum of the
	  actual frame is not passed (in this case we cannot shift out entirely the frame
	  because we don't know if it didn't pass the checksum because it's an actual valid frame
	  that was corrupted or a ghost frame).
	- To be sure to solve the problem, a good approach would be to a timeout at the receiver 
	side, the timeout starts whenever a HEAD is detected and when it expires the first head 
	byte is pulled out to continue searching for the next head without hangs.
	In our example this timeout will be simulated with an iterations counter.

	*/
	uint8_t TxData3[]={4,2,16,2,0,0,0,16,5,3,16,4,1,16,39,0,16,0,16,4,2};
	/*                     ^ Valid frame with right checksum
                                      ^ Valid frame which wrong checksum
									         ^ Ghost frame that can fit in Rx buffer
											         ^ Ghost frame which cannot fit in Rx buffer
													         ^ Frame with zero length
															      ^ Truncated frame
	*/

	//This is the rule that applies to this research:
	uint8_t head3[]={16};
	search_frame_rule rule3head = {
		.head=head2,
		.headLen=sizeof(head3),
		.tail=NULL,
		.tailLen=0,
		.minLen=1,
		.maxLen=0,
		.policy=soft
	};

	search_frame_rule rule3frame = {
		.head=head2,
		.headLen=sizeof(head3),
		.tail=NULL,
		.tailLen=0,
		.minLen=0, //to be replaced with LEN FIELD +2
		.maxLen=0,
		.policy=soft
	};

	printf("\n PART 3 -----------------------------------------------\n");

	initBuffers(TxData3,sizeof(TxData3));

	uint8_t currLen=0; //currently searhce dframe length
	uint8_t timeout=0; //timeout counter

	do{
		transmit(&TxBuff);
		
		printf("SEARCHING...\n");
		
		//searching for HEAD sequence followed by one byte
		while(searchFrameAdvance(&RxBuff,&FrameBuff,&rule3head,SHIFTOUT_FULL | SHIFTOUT_CURR)){ //we shift the buffer to the found HEAD byte
			timeout=0;
			currLen=cBuffReadByte(&FrameBuff,0,1);
			rule3frame.minLen=currLen+2;
			printf("FOUND HEADER, LEN FIELD: %u\n",currLen);
			while(1){
				//we implement only the timeout, if we want to speed up the research we can also use SHIFTOUT_FULL
				if(searchFrameAdvance(&RxBuff,&FrameBuff,&rule3frame,SHIFTOUT_NEXT)){
					//possible frame found
					printf("FOUND FRM:\t"); cBuffPrint(&FrameBuff,PRINTBUFF_NOEMPTY | PRINTBUFF_NONEWLINE);
					cBuffPull(&FrameBuff,NULL,1,0);
					cBuffPull(&FrameBuff,NULL,1,0);
					uint8_t check;
					cBuffPull(&FrameBuff,&check,1,1);
					printf("\tFRAME DATA:\t"); cBuffPrint(&FrameBuff,PRINTBUFF_NOEMPTY | PRINTBUFF_NONEWLINE);
					//frame validity check
					for(uint32_t pos=0;pos<FrameBuff.elemNum;pos++){
						check-=cBuffReadByte(&FrameBuff,0,pos);
					}
					if(check==0){
						printf("VALID FRAME!\n");
						break;
					}else{
						printf("INVALID FRAME.\n");
						break;
					}
				}else{
					timeout++;
					if(timeout==3){
						printf("TIMEOUT REACHED, DISCARDING FRAME.\n");
						cBuffPull(&RxBuff,NULL,1,0);
						break;
					}
					transmit(&TxBuff);
				}
			}
				printf("RX BUFFER:\t"); cBuffPrint(&RxBuff,0);
		}

		printf("NO MORE FRAMES FOUND\n");

	}while(!cBuffEmpty(&TxBuff));


	return 0;
}