#include "MTi1.h"

#ifdef DEBUG_MODE

#define DEBUG_PRINTF //comment this to disable debug printf even if DEBUG is defined

#endif

#define IMU_ACK_DELAY 100	//maximum time to wait for ack
#define IMU_CONFIG_RETRY 2 //number of times configuration commands will be sent if ack is not received


//structure to pass message data
//(excluding preamble, bid and crc)
typedef struct{
	uint8_t mid;
	uint8_t len;
	uint8_t* data;
} imu_packet_struct;

//defines
#define IMU_PREAMBLE 	0xfa
#define IMU_BID			0xff
//MID definitions
#define IMU_GOTO_CONFIG_MID			0x30
#define IMU_GOTO_CONFIG_ACK_MID		0x31
#define IMU_SET_OCONFIG_MID 		0xC0
#define IMU_SET_OCONFIG_ACK_MID		0xC1
#define IMU_GOTO_MEAS_MID 			0x10
#define IMU_GOTO_MEAS_ACK_MID		0x11
#define IMU_DATA_PACKET_MID 		0x36
//LEN definitions (for messages with LEN!=0)
#define IMU_SET_OCONFIG_LEN 		sizeof(outputConfigData)
#define IMU_SET_OCONFIG_ACK_LEN 	sizeof(outputConfigAckData)
#define IMU_DATA_PACKET_LEN			30
//data definitions
#define IMU_OUTPUT_CONFIG 		0x80, 0x20, 0x04, 0x80, /* Rate of turn */ \
								0xC0, 0x20, 0x04, 0x80 /* Magnetic Field */
#define IMU_OUTPUT_CONFIG_ACK 	0x80, 0x20, 0x04, 0x80, /* Rate of turn */ \
								0xC0, 0x20, 0x04, 0x80 /* Magnetic Field */
//others
#define IMU_DATA_GYRO_INDEX	3	//starting index (inside data) for gyroscope data
#define IMU_DATA_MAG_INDEX	18	//starting index (inside data) of magnetometer data

const uint8_t outputConfigData[]=		{IMU_OUTPUT_CONFIG};
const uint8_t outputConfigAckData[]=	{IMU_OUTPUT_CONFIG_ACK};

circular_buffer_handle rxcBuff; //rx and search buffer
uint8_t rxBuffer[IMU_BUFFER_LEN]; //memory buffer for rxBuff
uint8_t tmpBuff[IMU_BUFFER_LEN]; //temporary buffer where to store received packets

//function to compute message checksum
static uint8_t computeChecksum(imu_packet_struct * pckt){
	if(pckt==NULL) return 0;

	uint8_t crc=IMU_BID+pckt->mid+pckt->len;
	for(uint32_t d=0;d<pckt->len;d++){
		crc+=pckt->data[d];
	}
	return -crc;
}

//function to send message
static void sendMsg(UART_HandleTypeDef* IMUhandle, imu_packet_struct * pckt){
	if(pckt==NULL) return;

	uint8_t tmp=IMU_PREAMBLE;
	sendDriver_UART(IMUhandle, &tmp, 1);
	tmp=IMU_BID;
	sendDriver_UART(IMUhandle, &tmp, 1);
	sendDriver_UART(IMUhandle, &pckt->mid, 1);
	sendDriver_UART(IMUhandle, &pckt->len, 1);
	sendDriver_UART(IMUhandle, pckt->data, pckt->len);
	tmp=computeChecksum(pckt);
	sendDriver_UART(IMUhandle, &tmp, 1);
}

//function to receive next message, returns 1 if something was
//received, otherwise returns 0
//places the eventually received message inside pckt, if this is not needed pckt can be set NULL
//if checkCRC is !=0 the message CRC will be checked to approve the message
//NB. not checking crc is risky for many reasons but one of the worst is that
//len field could arrive corrupted so always check that len is the expected one
//format can be passed if a specific mid and len are required, otherwise can be left to NULL
//the buffer is automatically shifted out and filled at every call, user can eventually
//flush buffers before calling to get most recent messages
static uint8_t receiveMsg(UART_HandleTypeDef* IMUhandle, imu_packet_struct * pckt, imu_packet_struct* format, uint8_t checkCRC, uint32_t timeout){
	uint32_t startTick=HAL_GetTick();
	uint8_t len=0; //temporary variable to store target number of bytes to search
	uint8_t mid=0; //temporary variable to store message id

	typedef enum{
		_header,
		_packet
	} search_phase;

	search_phase phase=_header;

	uint8_t headTail[4]={IMU_PREAMBLE,IMU_BID,0,0};

	search_frame_rule rule;

	rule.head=(uint8_t *) headTail;
	rule.tail=NULL;
	rule.tailLen=0;
	rule.maxLen=0;
	rule.policy=soft;

	circular_buffer_handle foundPckt;
	imu_packet_struct tmpPckt;
	tmpPckt.data=tmpBuff;

	do{
		//filling buffer until is full or no more bytes available
		while(!cBuffFull(&rxcBuff)){ //fill buffer with new packets
			uint8_t c;
			if(receiveDriver_UART(IMUhandle, &c, 1)){
				cBuffPush(&rxcBuff, &c, 1,1);
			}else{
				break;
			}
		}

		//analyzing buffer
		if(phase==_header){ //if we are searching an header

			if(format!=NULL){	//if format specified
				mid=format->mid;
				len=format->len;
				phase=_packet;
				continue;	//jump to packet search
			}

			//search a complete xbus header (shiftOut disabled)
			rule.headLen=2;
			rule.minLen=2;
			if(searchFrameAdvance(&rxcBuff, &foundPckt, &rule, SHIFTOUT_FULL | SHIFTOUT_CURR | SHIFTOUT_FAST)){	//if we found a header, get MID and LEN fields
				mid=foundPckt.buff[2];
				len=foundPckt.buff[3];
				phase=_packet;
			}
		}else if(phase==_packet){
			headTail[2]=mid;
			headTail[3]=len;
			rule.headLen=4;
			rule.minLen=len+1;	//len+1 to house CRC

			//search for the complete packet with shiftOut active
			if(searchFrameAdvance(&rxcBuff, &foundPckt, &rule, SHIFTOUT_FULL | SHIFTOUT_NEXT | SHIFTOUT_FAST)){
#if enable_printf
				printf("RAW IMU FRAME:\n");
#endif
				//cBuffPrint(&foundPckt,PRINTBUFF_HEX | PRINTBUFF_NOEMPTY);

				tmpPckt.mid=mid;
				tmpPckt.len=len;
				if(len!=0) cBuffRead(&foundPckt,tmpPckt.data,foundPckt.elemNum,0,4);

				if(pckt!=NULL){ //if we want the packet to be output
					pckt->mid=mid;
					pckt->len=len;
					if(len!=0) pckt->data=tmpPckt.data;
				}

				//if crc must be checked
				if(checkCRC){
					if(cBuffReadByte(&foundPckt,1,0)==computeChecksum(&tmpPckt)){	//if correct crc
						return 1;
					}else phase=_header;	//continue search from next byte
				}else return 1;
#if enable_printf
				printf("Checksum verification failed!\n");
#endif
			}
		}else{
			phase=_header;	//in case of any state error, return to default state
		}

	}while((HAL_GetTick()-startTick) < timeout);

	return 0;
}

//function to send command to imu and wait for the right acknowledge
//the ack should be the next received or the transaction is considered failed
//returns 1 if ack received, 0 otherwise
static uint8_t imuAckTransaction(UART_HandleTypeDef* IMUhandle, imu_packet_struct * cmd, imu_packet_struct * ack, uint32_t timeout){
	if(cmd==NULL || ack==NULL) return 0;

	cBuffFlush(&rxcBuff); //flush circular buffer

	sendMsg(IMUhandle, cmd);

    if(receiveMsg(IMUhandle, NULL, ack, 1, timeout)){
    	return 1;
    }else{
    	return 0;
    }

}

uint8_t initIMUConfig(UART_HandleTypeDef* IMUhandle){
	//initializing circular buffer
	cBuffInit(&rxcBuff, rxBuffer, sizeof(rxBuffer),0);

    HAL_Delay(500);

    flushRXDriver_UART(IMUhandle);

    imu_packet_struct cmd;
    imu_packet_struct ack;

    cmd.len=0;
    ack.len=0;

    //going to config mode
    cmd.mid=IMU_GOTO_CONFIG_MID;
    ack.mid=IMU_GOTO_CONFIG_ACK_MID;
    for(uint32_t retry=0;retry<IMU_CONFIG_RETRY;retry++){
    	if(imuAckTransaction(IMUhandle,&cmd,&ack,IMU_ACK_DELAY)) break;
    	else if(retry==(IMU_CONFIG_RETRY-1)) return 0;
    }

    //set output config
    cmd.mid=IMU_SET_OCONFIG_MID;
    cmd.len=IMU_SET_OCONFIG_LEN;
    cmd.data=(uint8_t *)outputConfigData;
	ack.mid=IMU_SET_OCONFIG_ACK_MID;
	ack.len=IMU_SET_OCONFIG_ACK_LEN;
	for(uint32_t retry=0;retry<IMU_CONFIG_RETRY;retry++){
		if(imuAckTransaction(IMUhandle,&cmd,&ack,IMU_ACK_DELAY)) break;
		else if(retry==(IMU_CONFIG_RETRY-1)) return 0;
	}

    //go to measurement state
    cmd.mid=IMU_GOTO_MEAS_MID;
    cmd.len=0;
	ack.mid=IMU_GOTO_MEAS_ACK_MID;
	ack.len=0;
	for(uint32_t retry=0;retry<IMU_CONFIG_RETRY;retry++){
		if(imuAckTransaction(IMUhandle,&cmd,&ack,IMU_ACK_DELAY)) break;
		else if(retry==(IMU_CONFIG_RETRY-1)) return 0;
	}

	return 1;
}

// function to fill a 32 bit variable
// from a 4-byte wide buffer
uint32_t buff2Int32(uint8_t buff[4])
{
	uint32_t retval = 0;
	for (int b = 0; b < 4; b++)
	{
		retval = retval << 8;
		retval |= buff[b];
	}
	return retval;
}

/* Extract fields from IMU data field and converts it into host order before placing it inside data array*/
// frame is data field buffer, data is output data array, dataSize is number of data fields
void writeIMUDataArray(uint8_t* frame, uint32_t* data, uint32_t dataSize)
{
	for(uint32_t d=0;d<dataSize;d++){
		uint32_t raw=0;
		for(uint32_t byte=0;byte<4;byte++){
			raw|=((uint32_t)frame[d*4+byte])<<(8*(3-byte));
		}
		data[d]=raw;
	}
	return;
}

uint8_t readIMUPacket(UART_HandleTypeDef* IMUhandle, float gyroscope[3], float magnetometer[3], uint32_t timeout)
{
	//flush driver rx buffer
	cBuffFlush(&rxcBuff); //flushing local buffer

	imu_packet_struct format={
		.mid=IMU_DATA_PACKET_MID,
		.len=IMU_DATA_PACKET_LEN,
	};

	imu_packet_struct meas;

	if(receiveMsg(IMUhandle,&meas, &format, 1, timeout)){
		//found packet
		//writing gyro data
		writeIMUDataArray(&meas.data[IMU_DATA_GYRO_INDEX], (uint32_t*)gyroscope, 3);
		//writing mag data
		writeIMUDataArray(&meas.data[IMU_DATA_MAG_INDEX], (uint32_t*)magnetometer, 3);

		return 1;
	}

	return 0;
}
