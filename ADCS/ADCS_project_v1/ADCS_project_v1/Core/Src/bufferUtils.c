/**
 * @file bufferUtils.c
 * @author Simone Bollattino (simone.bollattino@gmail.com)
 * 
 */

#include "bufferUtils.h"

//PRIVATE FUNCTIONS ---------------------------------------

/* circular buffer rotation IN MEMORY, differently from cBuffRotate() in which only the
 * valid elements are rotated, this fuction actually rotates the full memory array.
 * The function works by giving a cBuffer handle structure and the memory index where
 * we want the virtual index 0 to be moved.
 * 
 * The used algorythm has linear complexity instead of quadratic as a simple byte-by-byte shift would have
 */
void cBuffRotateMemory(circular_buffer_handle* handle,uint32_t newStartIndx){
	if(handle==NULL || handle->buffLen==0 || newStartIndx==handle->startIndex) return;

	newStartIndx=newStartIndx%handle->buffLen;

	uint32_t moved=0; //number of moved elements
	uint32_t startPos=handle->startIndex; //starting position of every iteration
	uint32_t pos=startPos;	//physical position

	//temporary circular buffer, this represents the rotated buffer
	circular_buffer_handle tmpHandle;
	tmpHandle.buffLen=handle->buffLen;
	tmpHandle.startIndex=newStartIndx;

	while(moved<handle->buffLen){
		uint8_t tmpNew, tmpOld;
		tmpNew=handle->buff[pos]; 	//taking new element from position
		handle->buff[pos]=tmpOld;	//copying old element to position
		tmpOld=tmpNew; //updating old element
		moved++;	//incrementing number of moved elements
		//computing next physical position
		//this is equivalent to getting the virtual position of the taken element
		//and then get the corresponding physical position on the rotated buffer
		pos=cBuffGetMemIndex(&tmpHandle, cBuffGetVirtIndex(handle,pos));
		if(pos==startPos){
			handle->buff[pos]=tmpOld;
			startPos=(startPos+1)%handle->buffLen;
			pos=startPos;
		}
	}

	handle->startIndex=newStartIndx;

	return;
}

//PUBLIC FUNCTIONS ----------------------------------------

void pBuffInit(plain_buffer_handle* handle, uint8_t* buff, uint32_t buffLen, uint32_t elemNum){
	if(handle==NULL || buff==NULL) return;

	handle->buff=buff;
	handle->buffLen=buffLen;
	handle->elemNum=elemNum;

	return;
}

#ifdef BUFFERUTILS_ENABLEPRINT 
void pBuffPrint(plain_buffer_handle* handle, uint8_t flags){
	if(handle==NULL) printf("Cannot print NULL buffer handle\n");
	if(flags & PRINTBUFF_METADATA)
		printf("EN: %u\tBL: %u\t\t",handle->elemNum, handle->buffLen);

	for(uint32_t b=0;b<handle->buffLen;b++){
		if(b<handle->elemNum){ //if number inside buffer
			if(b==0){ //if head element
				printf("|");
			}else printf(" ");
			
			if(flags & PRINTBUFF_HEX){
				printf("%02x",handle->buff[b]);
			}else{
				printf("%u",handle->buff[b]);
			}

			if(b==handle->elemNum-1){ //if tail element
				printf("|\t");
			}else{
				printf("\t");
			}
		}else{
			if(!(flags & PRINTBUFF_NOEMPTY))
				printf(" __\t");
		}
	}

	if(!(flags & PRINTBUFF_NONEWLINE))
		printf("\n");

}
#endif

uint32_t pBuffWrite(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->buffLen==0 || dataLen == 0 || off>=handle->elemNum) return 0;

	//creating temporary circular buffer to allow using cBuff utilities
	circular_buffer_handle tmpBuff;
	pBuffToCirc(&tmpBuff,handle);

	return cBuffWrite(&tmpBuff,data,dataLen,ht,off);
}

uint32_t pBuffRead(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->buffLen==0 || dataLen == 0 || off>=handle->elemNum) return 0;

	//creating temporary circular buffer to allow using cBuff utilities
	circular_buffer_handle tmpBuff;
	pBuffToCirc(&tmpBuff,handle);

	return cBuffRead(&tmpBuff,data,dataLen,ht,off);
}

void pBuffWriteByte(plain_buffer_handle* handle, uint8_t val, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->buffLen==0 || handle->elemNum==0  || off>=handle->elemNum) return;

	if(!ht) handle->buff[off]=val;
	else handle->buff[handle->elemNum-1-off]=val;

	return;
}

uint8_t pBuffReadByte(plain_buffer_handle* handle, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->buffLen==0 || handle->elemNum==0 || off>=handle->elemNum) return 0;

	if(!ht) return handle->buff[off];
	//else
	return handle->buff[handle->elemNum-1-off];
}

uint32_t pBuffPush(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht){
	if(handle==NULL || handle->buffLen==0 || dataLen == 0 || data==NULL) return 0;

	//computing the actual number of bytes that can be pushed
	uint32_t retVal=handle->buffLen-handle->elemNum;
	if(retVal==0) return 0; //avoid computation if buffer is full
	if(dataLen<retVal) retVal=dataLen;

	//creating temporary circular buffer to allow using cBuff utilities
	circular_buffer_handle tmpBuff;
	pBuffToCirc(&tmpBuff,handle);

	//using cBuff push to push data into buffer
	cBuffPush(&tmpBuff,data,retVal,ht);

	if(ht==0){
		//rotating back memory array to have startIndex=0
		cBuffRotateMemory(&tmpBuff,0);
	}

	//rewrite metadata on handle
	handle->elemNum=tmpBuff.elemNum;

	return retVal;
}

uint32_t pBuffPull(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht){
	if(handle==NULL || handle->buffLen==0 || dataLen==0) return 0;

	//computing the actual number of bytes that can be pulled
	if(handle->elemNum==0) return 0; //avoid computation if buffer is empty

	//creating temporary circular buffer to allow using cBuff utilities
	circular_buffer_handle tmpBuff;
	pBuffToCirc(&tmpBuff,handle);

	//using cBuff pull to pull data from buffer
	uint32_t retVal=cBuffPull(&tmpBuff,data,dataLen,ht);

	if(ht==0){
		//rotating back memory array to have startIndex=0
		cBuffRotateMemory(&tmpBuff,0);
	}
	
	//rewrite metadata on handle
	handle->elemNum=tmpBuff.elemNum;

	return retVal;
}

uint32_t pBuffCut(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->elemNum==0 || dataLen==0 || off>=handle->elemNum) return 0;

	//creating temporary circular buffer to allow using cBuff utilities
	circular_buffer_handle tmpBuff;
	pBuffToCirc(&tmpBuff,handle);

	return cBuffCut(&tmpBuff,data,dataLen,ht,off);
}

void pBuffFlush(plain_buffer_handle* handle){
	if(handle==NULL) return;

	handle->elemNum=0;

	return;
}

uint8_t pBuffFull(plain_buffer_handle* handle){
	if(handle==NULL) return 0;

	return (handle->elemNum == handle->buffLen);
}

uint8_t pBuffEmpty(plain_buffer_handle* handle){
	if(handle==NULL) return 0;

	return (handle->elemNum == 0);
}

void cBuffInit(circular_buffer_handle* handle, uint8_t* buff, uint32_t buffLen, uint32_t elemNum){
	if(handle== NULL || buff==NULL) return;

	handle->buff=buff;
	handle->buffLen=buffLen;
	handle->startIndex=0;
	handle->elemNum=elemNum;

	return;
}

#ifdef BUFFERUTILS_ENABLEPRINT 
void cBuffPrint(circular_buffer_handle* handle, uint8_t flags){
	if(handle==NULL) printf("Cannot print NULL buffer handle\n");

	if(flags & PRINTBUFF_METADATA)
		printf("EN: %u\tSI: %u\tBL: %u\t\t",handle->elemNum, handle->startIndex, handle->buffLen);

	//b is virtual index or memory index depending on flags
	for(uint32_t b=0;b<handle->buffLen;b++){
		uint32_t tmpIndx;
		if(flags & PRINTBUFF_MEMORY){
			tmpIndx=cBuffGetVirtIndex(handle,b);
		}else{
			tmpIndx=b;
		}
		if(tmpIndx<handle->elemNum){ //if number inside buffer
			if(tmpIndx==0){ //if head element
				printf("|");
			}else printf(" ");
			
			uint8_t byte;
			if(flags & PRINTBUFF_MEMORY){
				byte=handle->buff[b];
			}else{
				byte=handle->buff[cBuffGetMemIndex(handle,tmpIndx)];
			}

			if(flags & PRINTBUFF_HEX){
				printf("%02x",byte);
			}else{
				printf("%u",byte);
			}

			if(tmpIndx==handle->elemNum-1){ //if tail element
				printf("|\t");
			}else{
				printf("\t");
			}
		}else{
			if(!(flags & PRINTBUFF_NOEMPTY))
				printf(" __\t");
		}
	}

	if(!(flags & PRINTBUFF_NONEWLINE))
		printf("\n");

}
#endif

uint32_t cBuffGetVirtIndex(circular_buffer_handle* handle,uint32_t memIndex){
	//special case when startIndex==0 is not critical but present to speed-up execution
	//of plain buffer functions based on circular buffer functions
	if(handle==NULL || handle->buffLen==0) return memIndex;

	memIndex=memIndex % handle->buffLen;

	if(handle->startIndex<=memIndex){
		return memIndex-handle->startIndex;
	}else{
		return handle->buffLen-handle->startIndex+memIndex;
	}
}

uint32_t cBuffGetMemIndex(circular_buffer_handle* handle,uint32_t virtIndex){
	//special case when startIndex==0 is not critical but present to speed-up execution
	//of plain buffer functions based on circular buffer functions
	if(handle==NULL || handle->buffLen==0) return virtIndex;

	virtIndex=virtIndex % handle->buffLen;

	if(virtIndex<(handle->buffLen-handle->startIndex)){
		return handle->startIndex+virtIndex;
	}else{
		return virtIndex-(handle->buffLen-handle->startIndex);
	}
}

void cBuffPush(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht){
	if(handle==NULL || handle->buffLen==0 || dataLen==0 || data==NULL) return;

	uint32_t pushMemIndx;
	if(ht==0){ //push before head
		//we will start pushing from buffLen-1 virtual index
		pushMemIndx=cBuffGetMemIndex(handle,handle->buffLen-1);
	}else{ //push after tail
		//we will start pushing from elemNum virtual index
		pushMemIndx=cBuffGetMemIndex(handle,handle->elemNum);
	}

	for(uint32_t d=0;d<dataLen;d++){
		if(ht==0){ //push to head
			//we push data starting from buffer end, going backwards
			//handle->buff[pushMemIndx]=data[dataLen-1-d]; //not tested, push following inverse order
			handle->buff[pushMemIndx]=data[d];
			//if buffer was not full before pushing
			if(handle->elemNum<handle->buffLen){
				//increasing element number counter
				handle->elemNum++;
			}
			//new start index is the one we pushed into
			handle->startIndex=pushMemIndx;
			pushMemIndx=cBuffGetMemIndex(handle,handle->buffLen-1);
		}else{ //push to tail
			//we push data following the data buffer order
			handle->buff[pushMemIndx]=data[d];
			//if buffer was not full before pushing
			if(handle->elemNum<handle->buffLen){
				//increasing element number counter
				handle->elemNum++;
			}else{
				//moving start index to virtual index 1
				handle->startIndex=cBuffGetMemIndex(handle,1);
			}
			pushMemIndx=cBuffGetMemIndex(handle,handle->elemNum);
		}

	}

	return;
}

uint32_t cBuffPushToFill(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht){
	if(handle==NULL || handle->buffLen==0 || dataLen==0 || data==NULL) return 0;

	//compute the available space
	uint32_t available=handle->buffLen-handle->elemNum;

	//compute the minimum between available and dataLen
	if(dataLen<available) available=dataLen;

	//pushing bytes
	cBuffPush(handle,data,available,ht);

	return available;
}

uint32_t cBuffPull(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht){
	if(handle==NULL || handle->buffLen==0 || dataLen==0) return 0;

	//expoiting cBuffRead function to perform data reading
	uint32_t retVal=cBuffRead(handle,data,dataLen,ht,0);

	//updating start index and elements number
	if(ht==0){ //pull from head
		handle->startIndex=cBuffGetMemIndex(handle,retVal);
	}

	handle->elemNum=handle->elemNum-retVal;

	return retVal;
}

uint32_t cBuffWrite(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->elemNum==0 || dataLen==0 || off>=handle->elemNum) return 0;

	//determinign the actual number of bytes we can write
	uint32_t retVal;
	if(dataLen<=handle->elemNum-off){
		retVal=dataLen;
	}else{
		retVal=handle->elemNum-off;
	}

	if(data!=NULL){ //data writing happens only if data is not NULL
		for(uint32_t d=0; d<retVal; d++){
			if(ht==0){ //write from head
				//writing data starting from head
				handle->buff[cBuffGetMemIndex(handle,d+off)]=data[d];
			}else{ //write from tail
				//writing data starting from tail
				//handle->buff[cBuffGetMemIndex(handle,handle->elemNum-1-d)]=data[dataLen-1-d]; //inverted write order, not tested
				handle->buff[cBuffGetMemIndex(handle,handle->elemNum-1-d-off)]=data[d];
			}
		}
	}

	return retVal;
	
}

uint32_t cBuffRead(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->elemNum==0 || dataLen==0 || off>=handle->elemNum) return 0;

	//determinign the actual number of bytes we can read
	uint32_t retVal;
	if(dataLen<=handle->elemNum-off){
		retVal=dataLen;
	}else{
		retVal=handle->elemNum-off;
	}

	if(data!=NULL){ //data reading happens only if data is not NULL
		for(uint32_t d=0; d<retVal; d++){
			if(ht==0){ //read from head
				//reading data starting from head
				data[d]=handle->buff[cBuffGetMemIndex(handle,d+off)];
			}else{ //read from tail
				//reading data starting from tail
				//data[retVal-1-d]=handle->buff[cBuffGetMemIndex(handle,handle->elemNum-1-d)]; //inverted read order, not tested
				data[d]=handle->buff[cBuffGetMemIndex(handle,handle->elemNum-1-d-off)];
			}
		}
	}

	return retVal;
	
}

uint32_t cBuffCut(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->elemNum==0 || dataLen==0 || off>=handle->elemNum) return 0;

	//reading data (using cBuffRead)
	uint32_t readLen=cBuffRead(handle,data,dataLen,ht,off);

	//returning immediately if nothing to cut
	if(readLen==0) return readLen;

	//cutting data from buffer
	//determining the smaller portion of buffer to shift
	uint32_t shiftStart=0; //start of shift (virtual)
	uint32_t shiftDest=0; //shift destination (virtual)
	uint32_t shiftLen=0; //length of shift
	uint8_t shiftPiece=0; //shortest memory piece to shift (0:first 1:second)
	if(off<=(handle->elemNum-off-readLen)){ //first buffer piece is shortes
		shiftStart=(ht==0) ? off-1 : handle->elemNum-off;
		shiftDest=(ht==0) ? off-1+readLen : handle->elemNum-off-readLen;
		shiftLen=off;
		shiftPiece=(ht==0) ? 0 : 1;
	}else{ //second buffer piece is shortest
		shiftStart=(ht==0) ? off+readLen : handle->elemNum-off-1-readLen;
		shiftDest=(ht==0) ? off : handle->elemNum-off-1;
		shiftLen=handle->elemNum-off-readLen;
		shiftPiece=(ht==0) ? 1 : 0;
	}
	//actual shift
	if(!shiftPiece){ //shift first memory part forward
		for(uint32_t b=0;b<shiftLen;b++){
			handle->buff[cBuffGetMemIndex(handle,shiftDest-b)]=handle->buff[cBuffGetMemIndex(handle,shiftStart-b)];
		}
		//changing start index
		handle->startIndex=cBuffGetMemIndex(handle,readLen);

	}else{ //shift second memory part backwards
		for(uint32_t b=0;b<shiftLen;b++){
			handle->buff[cBuffGetMemIndex(handle,shiftDest+b)]=handle->buff[cBuffGetMemIndex(handle,shiftStart+b)];
		}
	}
	//changing elemnum
	handle->elemNum=handle->elemNum-readLen;

	return readLen;
}

uint32_t cBuffPushRead(circular_buffer_handle* dest, circular_buffer_handle* source, uint32_t len, uint8_t htDest, uint8_t htSource){
	if(dest==NULL || source==NULL || dest->buffLen==0 || source->buffLen==0 || len==0) return 0;

	//actual number of moved bytes
	uint32_t retVal=len;
	if(source->elemNum<retVal) retVal=source->elemNum;
	if((dest->buffLen-dest->elemNum)<retVal) retVal=dest->buffLen-dest->elemNum;

	//moving bytes
	uint8_t byte;
	for(uint32_t b=0;b<retVal;b++){
		cBuffRead(source,&byte,1,htSource,b);
		cBuffPush(dest,&byte,1,htDest);
	}

	return retVal;
}

uint32_t cBuffPushPull(circular_buffer_handle* dest, circular_buffer_handle* source, uint32_t len, uint8_t htDest, uint8_t htSource){
	if(dest==NULL || source==NULL || dest->buffLen==0 || source->buffLen==0 || len==0) return 0;

	//moving using cBuffPushRead
	uint32_t retVal=cBuffPushRead(dest,source,len,htDest,htSource);
	//decreasing element number and chaning startIndex of source
	source->elemNum-=retVal;
	if(!htSource) source->startIndex=cBuffGetMemIndex(source,retVal);

	return retVal;
}

void cBuffWriteByte(circular_buffer_handle* handle, uint8_t val, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->buffLen==0 || handle->elemNum==0 || off>=handle->elemNum) return;

	if(!ht) handle->buff[cBuffGetMemIndex(handle,off)]=val;
	else handle->buff[cBuffGetMemIndex(handle,handle->elemNum-1-off)]=val;

	return;
}

uint8_t cBuffReadByte(circular_buffer_handle* handle, uint8_t ht, uint32_t off){
	if(handle==NULL || handle->buffLen==0 || handle->elemNum==0 || off>=handle->elemNum) return 0;

	if(!ht) return handle->buff[cBuffGetMemIndex(handle,off)];
	//else
	return handle->buff[cBuffGetMemIndex(handle,handle->elemNum-1-off)];
}


void cBuffRotate(circular_buffer_handle* handle,uint8_t dir, uint32_t pos){
	if(handle==NULL || handle->buffLen==0 || handle->elemNum==0) return;

	//temporary struct to store rotated cBuffer data
	circular_buffer_handle tmpBuff;
	tmpBuff.buffLen=handle->buffLen;
	tmpBuff.elemNum=handle->elemNum;

	//computing the new head virtual index
	uint32_t newStartVIndx=pos%handle->elemNum;
	if(dir) newStartVIndx=handle->elemNum-newStartVIndx;

	tmpBuff.startIndex=cBuffGetMemIndex(handle,newStartVIndx);

	//we then need to move all elements to close the hole between old tail and old head
	//computing the number of elements from new head to old tail (no need to move them)
	uint32_t fixed=handle->elemNum-newStartVIndx;
	//moving elements of handle in the range [old head:new head virtual index-1] after the old tail
	for(uint32_t vIndx=0;vIndx<cBuffGetVirtIndex(handle,tmpBuff.startIndex);vIndx++){
		handle->buff[cBuffGetMemIndex(&tmpBuff,fixed+vIndx)]=handle->buff[cBuffGetMemIndex(handle,vIndx)];
	}

	//finally assigning the new start index to the buffer
	handle->startIndex=tmpBuff.startIndex;
	return;
}

void cBuffFlush(circular_buffer_handle* handle){
	if(handle==NULL) return;

	handle->elemNum=0;

	return;
}

uint8_t cBuffFull(circular_buffer_handle* handle){
	if(handle==NULL) return 0;

	return (handle->elemNum == handle->buffLen);
}

uint8_t cBuffEmpty(circular_buffer_handle* handle){
	if(handle==NULL) return 0;

	return (handle->elemNum == 0);
}

void cBuffToPlain(plain_buffer_handle* pHandle, circular_buffer_handle* cHandle){
	if(cHandle==NULL || pHandle==NULL) return;

	cBuffRotateMemory(cHandle, 0);

	pHandle->buff=cHandle->buff;
	pHandle->buffLen=cHandle->buffLen;
	pHandle->elemNum=cHandle->elemNum;

	return;
}

void pBuffToCirc(circular_buffer_handle* cHandle, plain_buffer_handle* pHandle){
	if(cHandle==NULL || pHandle==NULL) return;

	cHandle->buff=pHandle->buff;
	cHandle->buffLen=pHandle->buffLen;
	cHandle->elemNum=pHandle->elemNum;
	cHandle->startIndex=0;

	return;
}

void pBuffToPlain(plain_buffer_handle* dest, plain_buffer_handle* pHandle){
	if(dest==NULL || pHandle==NULL) return;

	dest->buff=pHandle->buff;
	dest->buffLen=pHandle->buffLen;
	dest->elemNum=pHandle->elemNum;
}

void cBuffToCirc(circular_buffer_handle* dest, circular_buffer_handle* cHandle){
	if(dest==NULL || cHandle==NULL) return;

	dest->buff=cHandle->buff;
	dest->buffLen=cHandle->buffLen;
	dest->elemNum=cHandle->elemNum;
	dest->startIndex=cHandle->startIndex;
}
