/**
 * @file frameUtils.c
 * @author Simone Bollattino (simone.bollattino@gmail.com)
 * 
 */

#include "frameUtils.h"
#include <stdio.h>

/* utility function that checks if byte at virtual index pos of circular buffer is part of a pattern patt (length pattLen)
 * returns 0 if it's not part of it, returns 1 if the byte is part of pattern but is not inside a complete occurrence of
 * it, returns 2 if byte is inside a complete occurrence of the pattern.
 * The function also accepts a NULL patt buffer or pattLen==0, in that case it will return always 0
 * The function will also set the integer pointed by indx to the index of the byte inside the pattern array
 * (if correspondance is found), if indx is not needed it can be set to NULL, if multiple possible indexes are found, 
 * the indxPolicy will determine if the lowest (indxPolicy==0) or highest (indxPolicy!=0) index is written.
 */
static uint8_t checkByteIsPartOfPattern(circular_buffer_handle* handle, uint32_t pos, uint8_t* patt, \
		uint32_t pattLen, uint32_t* indx, uint8_t indxPolicy){

	if(handle==NULL || handle->elemNum==0 || patt==NULL || pattLen==0 || pos>=handle->elemNum) return 0;

	//check if complete correspondance is possible
	if(handle->elemNum>=pattLen){
		
		//computing first and last possible shifts of patter wrt the given byte
		//(the shift corresponds to the index of the byte inside pattern)
		uint32_t startShift; //first shift of patt that is possible inside buff
		uint32_t endShift;	//last shift(+1) of patt that is possible inside buff

		//computing start and end shift
		//if there's no space for pattern until end of buffer (because we are too near the end of the buffer)
		if((handle->elemNum-pattLen)<pos) startShift=pos-(handle->elemNum-pattLen); else startShift=0;
		//if there's no space for pattern from begin of buffer (because we are too near the begin of the buffer)
		if(pos<(pattLen-1)) endShift=pos+1; else endShift=pattLen;


		for(uint32_t s=0;s<(endShift-startShift);s++){ //loop to check all possible shifts
			uint32_t shift;
			if(!indxPolicy) shift=startShift+s;
			else			shift=endShift-1-s;

			uint8_t complete=1;
			for(uint32_t p=0;p<pattLen;p++){
				if(cBuffReadByte(handle,0,pos-shift+p)!=patt[p]){
					complete=0;
				}
			}

			if(complete){ //complete correspondance found
				if(indx!=NULL) *indx=shift;
				return 2;
			}
		}
	}

	//check for partial correspondance
	for(uint32_t p=0;p<pattLen;p++){

		uint32_t shift;
		if(!indxPolicy) shift=p;
		else			shift=pattLen-1-p;

		if(cBuffReadByte(handle,0,pos)==patt[shift]){
			//partial correspondance found
			if(indx!=NULL) *indx=shift;
			return 1;
		}
	}

	return 0;
}

uint32_t searchFrame(circular_buffer_handle* stream, circular_buffer_handle* frame, search_frame_rule * rule){
	//guard checks
	if(stream==NULL || stream->buff == NULL || rule==NULL || stream->elemNum==0 || rule->headLen==0 || rule->head==NULL) return stream->elemNum;
	//If packet cannot fit in available bytes
	if(stream->elemNum<(rule->headLen+rule->minLen+rule->tailLen)) return stream->elemNum;

	//state machine states
	typedef enum{
	    _waiting,
	    _inside
	} machine_state;

	//variables and flags
	machine_state state=_waiting;	//decoding state machine state
    uint32_t startPos=0;	//temporary variable were we save the last byte of head

	uint8_t headPart=0;	//flag to signal if this byte is part of a partial (if 1) or complete (if 2) head
	uint8_t tailPart=0;	//flag to signal if this byte is part of a partial (if 1) or complete (if 2) tail
	uint32_t headIndex=0;	//variable that will contain the index of the byte inside head (if part of it)
	uint32_t tailIndex=0;	//variable that will contain the index of the byte inside tail (if part of it)
	uint8_t forbiddenByte=0; //flag to signal that current byte is of forbidden type (head/tail or parts of it depending on mode)

	uint8_t canBeFirst=0; //flag to signal if the current byte can be the first byte of a frame (last byte of head)
	uint8_t canBeLast=0; //flag to signal if the current byte can be the last byte of a frame
	uint8_t minLenFlag=0;//flag to signal that the minimum length prerequisite has been satisfied
	uint32_t currLen=0; //current packet length (head and tail excluded)

    if(rule->policy != hard && rule->policy!=medium) rule->policy=soft;	//correct eventual policy error

	//scanning each byte inside the buffer (avoiding to check the last tailLen bytes)
	for(int b=0;b<(stream->elemNum-rule->tailLen);b++){

		//checking if byte can be part of a head or be a head end
        headPart=checkByteIsPartOfPattern(stream, b, rule->head, rule->headLen, &headIndex, 1);

        //checking if byte can be part of a tail or be a tail begin
        //if mode is tail-less, tailPart will always remain 0
        tailPart=checkByteIsPartOfPattern(stream, b, rule->tail, rule->tailLen, &tailIndex, 0);

		//check if byte is forbidden byte
		//for readibility i divided the assignment in different lines
		forbiddenByte=(rule->policy!=soft) && (headPart==2 || tailPart==2 || ((rule->policy==hard) && (headPart==1 || tailPart==1)));

		//checking minLen and maxLen policies
		if(state==_inside){
			currLen=b-startPos;
		}else{
			currLen=0;
		}
		//the second part is to account for head bytes that are also last bytes
		minLenFlag=currLen>=rule->minLen;
		forbiddenByte=forbiddenByte || ((rule->maxLen!=0) && (currLen > rule->maxLen));

		//checking if current byte can be the first frame byte (last head byte)
		canBeFirst=(headPart==2) && (headIndex==(rule->headLen-1));

		//checking if current byte can be the last frame byte
		if(minLenFlag){
			if(rule->tail== NULL || rule->tailLen==0){//tail-less mode
				canBeLast=1; 
			}else{ //normal mode
				uint32_t tmpTailIndex;
				canBeLast=checkByteIsPartOfPattern(stream, b+1, rule->tail, rule->tailLen, &tmpTailIndex, 0);
				canBeLast=(canBeLast == 2) && (tmpTailIndex==0);
			}
		}

		//STATE MACHINE
		if(state==_waiting){ 		//if we are waiting for the frame to begin
			if(canBeFirst){ //if we found a frame start
				state=_inside;
				startPos=b;

				//we check if we already found a 0 length frame
				if(canBeLast){
					//frame found!

					//computing packet length and starting virtual index
					uint32_t tmpLen=rule->headLen;
					if(rule->tail!=NULL) tmpLen+=rule->tailLen;
					uint32_t startVIndex=startPos+1-rule->headLen;

					//filling output handle
					if(frame!=NULL){
						frame->buff=stream->buff;
						frame->buffLen=stream->buffLen;
						frame->elemNum=tmpLen;
						frame->startIndex=cBuffGetMemIndex(stream,startVIndex);
					}
					return startVIndex;
				}
			}
		}else if(state==_inside){

			/* checks priority:
			 * Forbidden bytes? -> discard frame and eventually restart with next possible frame
			 * Last frame byte? -> frame found!
			 */

			if(forbiddenByte){
				//discard frame and eventually restart with next possible frame
				//state back to waiting
				state=_waiting;
				//after this iteration b will be incremented to the byte next to the old head
				//this can be optimized by taking note of the first occurrence of canBeFirst
				//and jumping there but for now this is enough 
				b=startPos;

			}else if(canBeLast){
				//frame found!

				//computing packet length and starting virtual index
				uint32_t tmpLen=currLen+rule->headLen;
				if(rule->tail!=NULL) tmpLen+=rule->tailLen;
				uint32_t startVIndex=startPos+1-rule->headLen;

				//filling output handle
				if(frame!=NULL){
					frame->buff=stream->buff;
					frame->buffLen=stream->buffLen;
					frame->elemNum=tmpLen;
					frame->startIndex=cBuffGetMemIndex(stream,startVIndex);
				}
				return startVIndex;
			}
		}else{
			//to manage state errors (should never happen)
			state=_waiting;
		}

    }
	//no valid packet found :(
	return stream->elemNum;
}


uint8_t searchFrameAdvance(circular_buffer_handle* stream, circular_buffer_handle* frame, search_frame_rule * rule, uint8_t shiftFlags){
	if(stream==NULL || stream->buff == NULL || rule==NULL || stream->elemNum==0 || rule->headLen==0 || rule->head==NULL) return 0;

	uint32_t startVIndex;
	uint8_t found=0;

	//we start searching for the frame by using searchFrame
	startVIndex=searchFrame(stream, frame, rule);

	//if frame was found
	if(startVIndex!=stream->elemNum){
		found=1;
		//shift out depending on flags SHIFTOUT_CURR, SHIFTOUT_NEXT and SHIFTOUT_FOUND
		if(shiftFlags & (SHIFTOUT_CURR | SHIFTOUT_NEXT | SHIFTOUT_FOUND)){ 
			cBuffPull(stream, NULL, startVIndex, 0);
			if(shiftFlags & SHIFTOUT_FOUND) cBuffPull(stream, NULL, frame->elemNum, 0);
			else if(shiftFlags & SHIFTOUT_NEXT) cBuffPull(stream, NULL, 1, 0);
		}
	}else{
		//if no packet was found check for SHIFTOUT_FULL flag
		if((shiftFlags & SHIFTOUT_FULL) && cBuffFull(stream)) cBuffPull(stream, NULL, 1, 0);
	}

	//regardless of packet found or not, perform SHIFTOUT_FAST if requested
	if(shiftFlags & SHIFTOUT_FAST){
		//temporary rule for searcing next occurrence of first head byte
		search_frame_rule tmpRule={ 
			.head=rule->head,
			.headLen=1,
			.minLen=0,
			.maxLen=0,
			.tail=NULL,
			.tailLen=0,
			.policy=hard,
		};
		//search next occurrence of first head byte
		uint32_t nextHead=searchFrame(stream,NULL,&tmpRule);

		cBuffPull(stream, NULL, nextHead, 0);
	}

	return found;
}
