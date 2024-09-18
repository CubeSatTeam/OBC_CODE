/**
 * @file simpleDataLink.c
 * @author Simone Bollattino (simone.bollattino@gmail.com)
 * 
 */
#include "simpleDataLink.h"

#define FRAME_FLAG 0x7E
#define ESCAPE_FLAG 0x7D
#define INVERTBIT5(byte) (byte ^ 0x20) 

#define CRC_POLY 0x1021 //16 bit crc polynomial
#define CRC_INITIAL 0xFFFF //16 bit crc initial value

#define FRMCODE_DATA 0x00//code for data frame
#define FRMCODE_ACK 0x01//code for acknowledge frame

//counter used to generate unique hashes for identical frames
uint16_t hashCnt=0;

//rules for frame search
const uint8_t headTail=FRAME_FLAG;
search_frame_rule rule={
	.head=(uint8_t *)&headTail, //default to dataHead, but needs to be changed to search for ack	
	.headLen=1,
	.tail=(uint8_t *)&headTail,	
	.tailLen=1,
	.minLen=1,
	.maxLen=(SDL_MAX_PAY_LEN+2)*2,
	.policy=hard,
};

// NETWORK ORDERING -----------------------------------------------------------

void num16ToNet(uint8_t net[2], uint16_t num){
    net[0]=(uint8_t)((num>>8) & 0xFF);
    net[1]=(uint8_t)(num & 0xFF);
    return;
}

uint16_t netToNum16(uint8_t net[2]){
    return ((uint16_t)net[0]<<8) | ((uint16_t)net[1]);
}

// STUFFING -------------------------------------------------------------------

uint8_t doByteStuffing(circular_buffer_handle* data){
    if(data==NULL || data->buff==NULL || data->buffLen==0 || data->elemNum==0 || data->elemNum==data->buffLen) return 0;

    uint32_t elemNum=data->elemNum;

    while(elemNum){
        uint8_t tmpByte;
        uint32_t numByte;
        //pull byte from buffer head
        cBuffPull(data,&tmpByte,1,0);
        //check if character needs escaping
        if(tmpByte==FRAME_FLAG || tmpByte == ESCAPE_FLAG){
            uint8_t escape=ESCAPE_FLAG;
            //try pushing escape flag
            numByte=cBuffPushToFill(data,&escape,1,1);
            if(numByte==0) return 0; //buffer is full, operation failed
            //flip 5th byte bit
            tmpByte=INVERTBIT5(tmpByte);
        }
        //try pushing byte
        numByte=cBuffPushToFill(data,&tmpByte,1,1);
        if(numByte==0) return 0; //buffer is full, operation failed

        //decrement remaining bytes
        elemNum--;
    }

    return 1;
}

uint8_t undoByteStuffing(circular_buffer_handle* data){
    if(data==NULL || data->buff==NULL || data->buffLen==0 || data->elemNum==0) return 0;

    uint32_t elemNum=data->elemNum;

    while(elemNum){
        uint8_t tmpByte;
        //pull byte from buffer head
        cBuffPull(data,&tmpByte,1,0);
        elemNum--;
        if(tmpByte == FRAME_FLAG) return 0; //error, cannot have frame flag inside payload
        //check if it's an escape byte
        if(tmpByte == ESCAPE_FLAG){
			//if buffer is over we simply delete the escape flag
			//(this shouldn't happen in a properly stuffed buffer)
            if(elemNum==0) return 0;

            //pull byte from buffer head
            cBuffPull(data,&tmpByte,1,0);
            elemNum--;

            //flip 5th byte bit
            tmpByte=INVERTBIT5(tmpByte);

            //if a 7d is encountered without escaping anything
            if(tmpByte != ESCAPE_FLAG && tmpByte != FRAME_FLAG) return 0;

            //push byte on tail
            cBuffPushToFill(data,&tmpByte,1,1);

        }else{
			//push byte on tail
			cBuffPushToFill(data,&tmpByte,1,1);
		}
    }
    
    return 1;
}

// CRC/HASH -------------------------------------------------------------------

/*
//this function can be used to verify other CRC implementations
//this should not be used in production in favor of the LUT implementation
uint16_t computeCRC(circular_buffer_handle* dataBuff){
	if(dataBuff==NULL || dataBuff->buff==NULL) return 0;

	const uint16_t poly=CRC_POLY;
	const uint16_t initVal=CRC_INITIAL;

	uint16_t crc=initVal;

	for(uint32_t b=0;b<dataBuff->elemNum;b++){
		uint16_t byte=((uint16_t)cBuffReadByte(dataBuff,0,b));

		crc=(byte << 8) ^ crc;

		for(uint8_t bit=0;bit<8;bit++){
			if(crc & 0x8000){
				crc=(crc<<1) ^ poly;
			}else{
				crc=crc<<1;
			}
		}
	}

	return crc;

}
*/

/*
//this function can be used to print a CRC LUT on the terminal
#include <stdio.h>
void printCRCLUT(){
    uint16_t poly=CRC_POLY;

	uint8_t byteVal=0;
	printf("const uint16_t CRCLUT%04x[]={\n",poly);
	do{
		uint16_t crc=byteVal<<8;
		for(uint8_t bit=0;bit<8;bit++){
			if(crc & 0x8000){
				crc=(crc<<1) ^ poly;
			}else{
				crc=crc<<1;
			}
		}
		printf("0x%04x, ",crc);
		byteVal++;
		if(!(byteVal%16)) printf("\n");
	}while(byteVal!=0);
	printf("};");
}
*/

const uint16_t CRCLUT1021[]={
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

uint16_t computeCRCwithLUT(circular_buffer_handle* dataBuff){
	if(dataBuff==NULL || dataBuff->buff==NULL) return 0;

	const uint16_t initVal=CRC_INITIAL;

	uint16_t crc=initVal;

	for(uint32_t b=0;b<dataBuff->elemNum;b++){
		uint16_t byte=((uint16_t)cBuffReadByte(dataBuff,0,b));

		crc=(byte<<8) ^ crc;

		crc=(crc<<8) ^ CRCLUT1021[(uint8_t) (crc>>8)];
	}

	return crc;
}

uint8_t addCRC(circular_buffer_handle* data){
    if(data==NULL || data->buff==NULL || data->buffLen==0) return 0;

    uint16_t CRC=computeCRCwithLUT(data);
    //append crc to frame (network order)
    uint8_t tmpCRC[2];
    num16ToNet(tmpCRC,CRC);
    if(cBuffPushToFill(data,tmpCRC,2,1) == 2) return 1;
    //else
    return 0;
}

uint8_t removeVerifyCRC(circular_buffer_handle* data){
    if(data==NULL || data->buff==NULL || data->buffLen==0 || data->elemNum<2) return 0;

    //compute CRC (should be 0)
    uint16_t CRC=computeCRCwithLUT(data);
    //pull CRC bytes from buffer
    cBuffPull(data,NULL,2,1);

    if(!CRC) return 1;
    //else
    return 0;
}

/* this function computes an hash starting from a buffer of data
 * right now it simply returns the value of the hash counter to
 * generate the hash, it can be modified to implement more robust
 * types of hashes but in our case we will only use it to identify
 * frames uniquely for acknowledges so it should be good enough
 */
uint16_t computeHash(uint8_t * hashData, uint32_t dataLen){
    return ++hashCnt;
}

// FRAME/DEFRAME FUNCTIONS ----------------------------------------------------

/*
 * @brief Function to frame a payload.
 * 
 * This function takes a payload under the form of a bufferUtils circular
 * buffer and frames it inside a frame with the following format:
 * 
 * |FLAG| PAYLOAD | CRC16 |FLAG|
 * 
 * Where the flag is 0x7E and the CRC16 is computed by using 0x1021 and
 * initial value of 0xFFFF, the CRC is appended to the frame in network
 * order (big endian) regardless of the architecture.
 * 
 * The function then performs an HDLC-like byte stuffing: the flag byte
 * 0x7E and the escape byte 0x7D are replaced by 0x7D followed by the
 * original one in which the 5-th but has been flipped (so basically
 * 0x7E -> 0x7D 0x5E and 0x7D -> 0x7D 0x5D), this ensures that the flag
 * byte cannot be inside the frame if not at the begin and end.
 * 
 * Finally the function appends the FLAG byte at the begin and end of the
 * frame, which at this point is ready to be transmitted.
 * 
 * NB: The frame is built inside the payload buffer and this has the following
 * consequences:
 * - the memory array of the buffer should be long enough to fit all the added
 * bytes, the worst case is len(PAYLOAD)*2+6, if the buffer is too small the
 * function will return 0 to signal that the operation failed
 * - the operation is destructive, even if the operation failed, the content is
 * corrupted.
 * 
 * @param payload circular buffer handle containing the payload and inside
 *                which the frame will be built
 * @return uint8_t 0 if an error occurred (buffer too small), !0 otherwise
 */
uint8_t frame(circular_buffer_handle * payload){
    if(payload==NULL || payload->buff==NULL) return 0;

    //add crc to buffer
    if(!addCRC(payload)) return 0;

    //perform byte stuffing
    if(!doByteStuffing(payload)) return 0;

    //add head and tail
    uint8_t flag=FRAME_FLAG;
    if(!cBuffPushToFill(payload,&flag,1,0)) return 0;
    if(!cBuffPushToFill(payload,&flag,1,1)) return 0;

    return 1;
}

/*
 * @brief Function to de-frame a payload.
 * 
 * This function implements the exact opposite operation of frame(), it removes
 * the FLAG bytes from the begin and end of frame, then it reverts the byte 
 * stuffing, verifies the CRC and returns an error if any of those operations
 * failed.
 * 
 * The function can fail if:
 * - The frame doesn't begin and/or end with 0x7E
 * - The frame contains 0x7E or 0x7D not followed by 0x5E or 0x5D
 * - The CRC verification failed
 * 
 * @param frame circular buffer handle containing the frame and inside which
 *              the payload will be written
 * @return uint8_t 0 if an error occurred, !0 otherwise
 */
uint8_t deframe(circular_buffer_handle * frame){
    if(frame==NULL || frame->buff==NULL) return 0;

    //remove head and tail
    uint8_t flag=0;
    if(!cBuffPull(frame,&flag,1,0)) return 0;
    if(flag!=FRAME_FLAG) return 0;
    if(!cBuffPull(frame,&flag,1,1)) return 0;
    if(flag!=FRAME_FLAG) return 0;

    //remove byte stuffing
    if(!undoByteStuffing(frame)) return 0;

    //remove and verify CRC
    if(!removeVerifyCRC(frame)) return 0;

    return 1;
}

// BASIC I/O FUNCTIONS --------------------------------------------------------
//sends a frame on line txBuff
uint8_t sendFrame(serial_line_handle* line, uint8_t frameCode, uint8_t ackWanted, uint16_t hash, uint8_t* buff, uint32_t len){
    if(line==NULL || line->txFunc==NULL) return 0;

    if(len>SDL_MAX_PAY_LEN) return 0;

    //initializing temporary circular buffer
    cBuffInit(&line->tmpBuff,line->tmpBuffArray,sizeof(line->tmpBuffArray),0);

    //creating frameHeader
    frameHeader header={
        .code=frameCode,
        .ackWanted=ackWanted,
        .hash=hash
    };

    //network ordering header
    num16ToNet((uint8_t*)&header.hash,header.hash);

    //copying header inside circular buffer
    if(cBuffPushToFill(&line->tmpBuff,(uint8_t *)&header,sizeof(frameHeader),1)!=sizeof(frameHeader)) return 0;

    //copying data inside circular buffer
    if(buff!=NULL) if(cBuffPushToFill(&line->tmpBuff,buff,len,1)!=len) return 0;

    //framing the payload
    if(!frame(&line->tmpBuff)) return 0;

    //sending the payload through the line
    uint8_t byte;
    while(cBuffPull(&line->tmpBuff,&byte,1,0)){
        //if the transmission fails, return 0
        if(!line->txFunc(byte)) return 0;
    }
 
    return 1;
}

//receives a frame from line rxBuff, searching for a certain frameCode, if some remCodes are specified (not NULL or empty)
//it also removes those codes from rxBuff, otherwise it leaves them unchanged
//the eventually received frame will be placed inside line tmpBuff (HEADER INCLUDED!)
//returns 0 if no frame found, !0 otherwise
uint8_t receiveFrame(serial_line_handle* line, uint8_t frameCode, circular_buffer_handle* remCodes){
    if(line==NULL || line->rxFunc==NULL) return 0;

    //initializing temporary circular buffer
    cBuffInit(&line->tmpBuff,line->tmpBuffArray,sizeof(line->tmpBuffArray),0);

    //fill the rxBuffer with new bytes
    uint8_t byte;
    while(!cBuffFull(&line->rxBuff)){
        if(line->rxFunc(&byte)){
            cBuffPush(&line->rxBuff,&byte,1,1);
        }else break;
    }
   
    //handle to store found frames
    circular_buffer_handle frameHandle;
    //dummy buffer to perform buffer advancement
    circular_buffer_handle dummyBuff;
    //copying rxBuff into dummy buffer
    cBuffToCirc(&dummyBuff,&line->rxBuff);
    //we search on dummy handle, shifting it out to current found frame
    while(searchFrameAdvance(&dummyBuff,&frameHandle,&rule,SHIFTOUT_NEXT | SHIFTOUT_FAST)){
        //flush tmp buffer
        cBuffFlush(&line->tmpBuff);
        //copy on temporary buffer
        cBuffPushRead(&line->tmpBuff,&frameHandle,frameHandle.elemNum,1,0);
        //try deframing
        if(!deframe(&line->tmpBuff)) continue;

        //check if it corresponds to wanted frame code
        frameHeader tmpHeader;
        uint8_t toBeCut=0; //flag to signal that frame needs to be cut from rxBuff
        uint8_t found=0; //frame found flag
        //reading header
        if(cBuffRead(&line->tmpBuff,(uint8_t *)&tmpHeader,sizeof(frameHeader),0,0)!=sizeof(frameHeader)) continue;
        //host ordering header
        tmpHeader.hash=netToNum16((uint8_t*)&tmpHeader.hash);
        if(line->tmpBuff.elemNum>(SDL_MAX_PAY_LEN+sizeof(frameHeader))) continue;
        if(tmpHeader.code==frameCode){
            //frame found
            toBeCut=1;
            found=1;
        }else{
            if(remCodes!=NULL){
                for(uint32_t c=0; c<remCodes->elemNum; c++){
                    if(cBuffReadByte(remCodes,0,c)==tmpHeader.hash){
                        toBeCut=1;
                        break;
                    }
                }
            }
        }

        if(toBeCut){ //if frame needs to be cut
            //we cut the found frame from buffers
            //saving the virtual index of the found frame inside rxBuff
            uint32_t frameIndx=cBuffGetVirtIndex(&line->rxBuff,frameHandle.startIndex);
            //cutting found frame from rxBuff
            cBuffCut(&line->rxBuff,NULL,frameHandle.elemNum,0,frameIndx);
            //reconstructing dummy buffer
            cBuffToCirc(&dummyBuff,&line->rxBuff);
            cBuffPull(&dummyBuff,NULL,frameIndx+1,0);
        }

        if(found) return 1;
    }

    return 0;
}

//COMPLEX I/O FUNCTIONS -------------------------------------------------------

//receive a frame and eventually acknowledge it
//returns the length of frame if received, 0 otherwise
//searches for a frame with code frameCode, and eventually removes remCodes frames from rxBuff (if not NULL or empty)
//pushes the received code in rxFrame tail, if not NULL, ONLY pushing if there's enough space
//if there's not enough space to store the frame, the ack is not sent even if requested
uint32_t receiveFrameAndAck(serial_line_handle* line, circular_buffer_handle* rxFrame, uint8_t frameCode, circular_buffer_handle* remCodes){
    if(line==NULL || line->rxFunc==NULL) return 0;
    
    //if frame received
    if(receiveFrame(line, frameCode,remCodes)){
        //get header
        frameHeader tmpHeader;
        cBuffPull(&line->tmpBuff,(uint8_t *)&tmpHeader,sizeof(frameHeader),0);
        //host ordering header
        tmpHeader.hash=netToNum16((uint8_t*)&tmpHeader.hash);

        uint8_t sendAck=1;
        uint32_t len=line->tmpBuff.elemNum;
        //verify if the frame was already received
        if(tmpHeader.hash == line->lastRxHash){
            len=0; 
        }else{
            if(rxFrame!=NULL){
                //pushing it on buffer (if enough space)
                if((rxFrame->buffLen-rxFrame->elemNum)>=len){
                    cBuffPushPull(rxFrame, &line->tmpBuff, len, 1,0);
                }else sendAck=0;
            }
        }

        //send ack back if needed (if ack sending fails it's considered as lost on the line, the frame is received anyway)
        if(tmpHeader.ackWanted && sendAck){ 
            sendFrame(line, FRMCODE_ACK, 0, tmpHeader.hash,NULL,0);
            //saving last acknowledged hash
            line->lastRxHash=tmpHeader.hash;
        }

        return len;
    }

    return 0;

}

//tries receiving a single ack with the given hash
//to be called multiple times to scan the whole buffer
uint8_t receiveAck(serial_line_handle* line, uint16_t hash){
    if(line==NULL || line->rxFunc==NULL) return 0;

    if(receiveFrame(line,FRMCODE_ACK,NULL)){
        //get header
        frameHeader tmpHeader;
        cBuffPull(&line->tmpBuff,(uint8_t *)&tmpHeader,sizeof(frameHeader),0);
        //host ordering header
        tmpHeader.hash=netToNum16((uint8_t*)&tmpHeader.hash);
        //check if hash correct
        if(tmpHeader.hash == hash) return 1;
    }

    return 0;

}

#ifdef SDL_ANTILOCK_DEPTH
//receives frames placing them inside anti lock queue (and eventually responding with an ack)
//returns 0 in case of failure, length of frame otherwise
uint32_t receiveInQueueAndAck(serial_line_handle* line, uint8_t frameCode, circular_buffer_handle* remCodes){
    if(line==NULL || line->rxFunc==NULL) return 0;

    //check if there's space in antiLockQueue
    if(line->alockQueue.elemNum==line->alockQueue.buffLen) return 0;

    //otherwise try receiving a frame
    uint32_t len=receiveFrameAndAck(line,&line->alockBuff, frameCode, remCodes);

    //if frame received
    if(len){
        cBuffPush(&line->alockQueue,(uint8_t*)&len,sizeof(line->tmpBuff.elemNum),1);
    }

    return len;
}

//reads a frame from anti lock queue
//returns length of frame, otherwise 0
//places frame inside rxFrame (if not null), only if there's enough space
uint32_t readFromQueue(serial_line_handle* line, circular_buffer_handle* rxFrame){
    if(line==NULL) return 0;

    //check if there are frames in queue
    uint32_t frameLen=0;
    cBuffPull(&line->alockQueue,(uint8_t*)&frameLen,sizeof(frameLen),0);

    //read frame from queue
    if(rxFrame!=NULL){
        //pushing it on buffer (if enough space)
        uint32_t len=frameLen;
        if((rxFrame->buffLen-rxFrame->elemNum)>=len){
            cBuffPushPull(rxFrame, &line->alockBuff, len, 1,0);
        }else{
            //otherwise just discard the frame
            cBuffPull(&line->alockBuff,NULL,len,0);
            return 0;
        }
    }

    return frameLen;
}
#endif

// SIMPLE DATA LINK FUNCTIONS -------------------------------------------------
void sdlInitLine(serial_line_handle* line, uint8_t (*txFunc)(uint8_t byte), uint8_t (*rxFunc)(uint8_t* byte), uint32_t timeout, uint32_t retries){
    if(line==NULL) return;

    line->txFunc=txFunc;
    line->rxFunc=rxFunc;
    cBuffInit(&line->rxBuff,line->rxBuffArray,sizeof(line->rxBuffArray),0);
    line->timeout=timeout;
    line->retries=retries;
    line->lastRxHash=0;
    cBuffInit(&line->tmpBuff,line->tmpBuffArray,sizeof(line->tmpBuffArray),0);

#ifdef SDL_ANTILOCK_DEPTH
    cBuffInit(&line->alockBuff,line->alockBuffArray,sizeof(line->alockBuffArray),0);
    cBuffInit(&line->alockQueue,line->alockQueueArray,sizeof(line->alockQueueArray),0);
#endif
}

uint8_t sdlSend(serial_line_handle* line, uint8_t* buff, uint32_t len, uint8_t ackWanted){
    if(line==NULL || line->txFunc==NULL || buff==NULL || len==0) return 0;

    if(len>SDL_MAX_PAY_LEN) return 0;

    //generating hash
    uint16_t hash=computeHash(buff,len);

    //try sending frame
    uint32_t retryNum=0;
    do{
        retryNum++;
        
        //send data
        if(!sendFrame(line,FRMCODE_DATA,ackWanted,hash,buff,len)) continue;

        if(!ackWanted) return 1;

#ifdef SDL_DEBUG
        __sdlTestSendCallback(line);
#endif

        //saving starting tick for timeout
        uint32_t startTick=sdlTimeTick();
        do{
            if(receiveAck(line,hash)) return 1;

#ifdef SDL_ANTILOCK_DEPTH
        //if anti lock active, fill the queue while waiting
        receiveInQueueAndAck(line,FRMCODE_DATA,NULL);
#endif
        }while((sdlTimeTick()-startTick)<=line->timeout);

    }while(retryNum<=line->retries);

    return 0;
}

uint32_t sdlReceive(serial_line_handle* line, uint8_t* buff, uint32_t len){
    if(line==NULL || line->rxFunc==NULL) return 0;

    uint32_t retVal=0;

    //temporary cBuffer
    circular_buffer_handle dummyHandle;
    cBuffInit(&dummyHandle, buff, len,0);

#ifdef SDL_ANTILOCK_DEPTH
    //try reading from queue
    retVal=readFromQueue(line, &dummyHandle);
    if(retVal) return retVal;
#endif

    //otherwise try receiving a fresh frame
    uint8_t remCode[]={FRMCODE_ACK}; //we remove old acks from buffer
    circular_buffer_handle remCodes;
    cBuffInit(&remCodes,remCode,sizeof(remCode),sizeof(remCode));
    retVal=receiveFrameAndAck(line,&dummyHandle,FRMCODE_DATA,&remCodes);

    return retVal;
}
