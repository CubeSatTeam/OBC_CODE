/**
 * @file frameUtils.h
 * @author Simone Bollattino (simone.bollattino@gmail.com)
 * @brief Functions to search frames inside serial buffers.
 * 
 */

#ifndef FRAMEUTILS_H
#define FRAMEUTILS_H

#include "bufferUtils.h"

/**
 * @brief policy for frame search.
 * 
 * Applied in cases where parts of head or tail sequences are found inside
 * the frame.
 */
typedef enum{
	hard,		///< hard policy, even a part of head/tail will make the function reject the frame
	medium,		///< medium policy, a part of head/tail will not make the frame rejected but a complete head or tail will
	soft,		///< soft policy, the frame can contain bytes that are part of head/tail, also complete heads/tails
} head_tail_policy;

/**
 * @brief Directives for frame search.
 * 
 */
typedef struct{
	uint8_t * head;		///< array with frame head sequence
	uint32_t headLen;	///< length of head sequence
	uint8_t * tail;		///< array with frame tail sequence
	uint32_t tailLen;	///< length of tail sequence
	uint32_t minLen;	///< minimum frame length (head and tail excluded)
	uint32_t maxLen;	///< maximum frame length (head and tail excluded)
	head_tail_policy policy;	///< policy for partial heads' or tails' bytes inside frame
} search_frame_rule;

/**
 * @brief Function to search a frame inside a circular buffer.
 * 
 * This is a quite complex but versatile function which can be used to search
 * frames inside a stream represented by a circular buffer of buffUtils.h
 * A frame is represented by a sequence of bytes starting with a certain head
 * sequence and finishing with a certain tail sequence, as in this example:
 * 
 * @verbatim
 * |  HEAD   |      FRAME BYTES       |TAIL|
 * | H0 | H1 | F0 | F1 | F2 | F3 | F4 | T0 |
 * @endverbatim
 * 
 * SEARCH RULES
 * The function is configured by passing a search_frame_rule structure which 
 * contains a number of parameters to change its behavior:
 * - head and tail arrays containing the head and tail sequences and relative
 *   lengths headLen and tailLen
 * - minLen value which is the minimum length of the FRAME BYTES part of the
 *   frame (so excluding head and tail sequences)
 * - maxLen value which in the maximum length of the FRAME BYTES part of the
 *   frame (so excluding head and tail sequences), the maxLen value is included
 *   so the FRAME BYTES part can be long up to maxLen but not more than that.
 *   If you don't want to use the maxLen value, you can put it to 0 and it will
 *   be ignored.
 * - policy, which is of type head_tail_policy and describes what the function
 *   should do if it encounters bytes that are part of head or tail sequence
 *   inside the FRAME BYTES part of the frame, there are three possible
 *   policies: soft, medium and hard.
 *   
 *   SEARCH POLICY
 *    - The soft policy allows both partial (some bytes) or full head/tail 
 *      sequences to be inside the frame, here are some examples of frames
 *      that are valid with soft policy:
 * 
 *      @verbatim
 *      |  HEAD   |      FRAME BYTES       |TAIL|
 *      | H0 | H1 | H1 | H1 | X  | X  | X  | T0 |
 *      | H0 | H1 | X  | X  | H0 | H1 | X  | T0 |
 *      | H0 | H1 | X  | X  | X  | T0 | X  | T0 | (called with minLen > 3)
 *      @endverbatim
 *
 *    - The medium policy allows only partial (some bytes) head/tail sequences
 *      to be inside the frame, for example:
 * 
 *      @verbatim
 *      |  HEAD   |      FRAME BYTES       |  TAIL   |
 *      | H0 | H1 | X  | X  | X  | X  | X  | T0 | T1 |
 *      | H0 | H1 | X  | H0 | T1 | X  | X  | T0 | T1 | (with H0!=T0 && T1!=H1)
 *      | H0 | H1 | X  | X  | X  | T0 | X  | T0 | T1 |
 *      @endverbatim
 * 
 * 		NB. a byte is part of a complete head sequence even if a possible head
 *      was already found and the sequence is interleaved with the previous, as
 *      in this example (function called with head sequence | H0 | H0 | and 
 *      medium policy, maxLen > 5):
 * 
 *      @verbatim
 * 		| H0 | H0 | H0 | X  | X  | X  | X  | T0 |
 * 		this is a possible head sequence with corresponding frame:
 *      |  HEAD   |      FRAME BYTES       |TAIL|
 * 		but then the function finds the third H0, this is still considered a
 *      part of a complete head sequence, so the frame which is actually found
 *      is the following:
 *           |  HEAD   |   FRAME BYTES     |TAIL|
 *      To make the function find the first one you should call it with soft
 *      policy (but make sure that there aren't complete head sequences inside
 *      FRAME BYTES so the behavior is not the same)
 *      @endverbatim

 *    - The hard policy doesn't allow bytes which are part of head and tail to
 *      be inside the frame, this policy is usually coupled with byte stuffing
 *      techniques at higher layers to be sure that those bytes don't appear 
 *      inside the FRAME BYTES part.
 * 
 * Whenever the function finds a frame which violates the rules, it will start
 * searching for the next one beginning from immediately after the previous head
 * sequence, as in this example (function called with minLen=2, medium 
 * policy and head sequence equal to tail sequence -> |H/T0|H/T1|  ):
 * 
 * @verbatim
 * Searching inside this buffer:
 * |H/T0|H/T1| X  |H/T0|H/T1| X  |H/T1| X  |H/T0|H/T1|
 * found this possible frame, but it's too short:
 * |  HEAD   | F0 |  TAIL   |
 * finally found this valid frame:
 *                |  HEAD   | F0 | F1 | F2 |  TAIL   |
 * The same search called with soft policy will instead find this frame:
 * |  HEAD   | F0 | F1 | F2 | F3 | F4 | F5 |  TAIL   |
 * While hard policy will not find any frame.
 * @endverbatim
 * 
 * FUNCTION MODES
 * The function can work in two major modes: normal mode and tail-less.
 * Normal mode is the one that was explained until now, with frames having both
 * head and tail sequences, while tail-less mode is for frames which only have 
 * head sequences, tail-less mode is activated by passing a NULL tail array or
 * a tailLen value of 0, in this mode the function will search for a frame wich
 * starts with the head sequence and with minLen length, as in the example:
 * 
 * @verbatim
 * |  HEAD   |      FRAME BYTES       | (function called with minLen=5)
 * | H0 | H1 | F0 | F1 | F2 | F3 | F4 | X  | X  | X  | .....
 * @endverbatim
 * 
 * Beside that, all the other rules are still valid.
 * Calling the function with head=NULL or headLen=0 is not an option and
 * will result in no framess being found.
 * 
 * Special case: in tail-less mode, the function can also be used to perform 
 * pattern search, if it's called with minLen=0 it will search the | HEAD |
 * sequence alone inside the frame.
 * 
 * RETURN DATA
 * The function will return the starting virtual index of the eventually found 
 * frame inside stream buffer (head and/or tail lengths included), otherwise
 * it will return stream->elemNum if no frame was found.
 * The function will also make the frame circular buffer handle to contain ONLY
 * the found frame for reading (so the frame will start at virtual index 0 and
 * stop at virtual index elemNum-1 of frame handle, head and tail included).
 * NB. the memory array of stream will be ASSIGNED as the memory array pointer
 * of frame, meaning that frame circular buffer handle doesn't need to be 
 * initialized before AND that you should read out the frame from the circular
 * buffer before performing other operations on stream that could overwrite
 * the memory locations containing the found frame (for example pushing new
 * bytes that would overwrite older ones).
 * 
 * @param stream circular buffer where to search the frame 
 * @param fram output circular buffer where the found frame will be returned
 * @param rule set of rules to configure the search
 * @return uint32_t starting virtual index of found frame (first head byte)
 *                  otherwise it will return stream->elemNum
 */
uint32_t searchFrame(circular_buffer_handle* stream, circular_buffer_handle* frame, search_frame_rule * rule);

/**
 * @brief Flag to not perform any shift of buffer.
 * 
 */
#define SHIFTOUT_NONE		0x00
/**
 * @brief Flag to perform a single byte shift if buffer is full.
 * 
 */	
#define SHIFTOUT_FULL		0x01
/**
 * @brief Flag to shift buffer until the start of the found frame.
 * 
 */	
#define SHIFTOUT_CURR		0x02
/**
 * @brief Flag to shift buffer until the start of the found frame plus 1.
 * 
 */	
#define SHIFTOUT_NEXT		0x04
/**
 * @brief Flag to shift buffer until the start of the found frame plus 1.
 * 
 */	
#define SHIFTOUT_FOUND		0x08
/**
 * @brief Flag to shift buffer until the next occurrence of the first byte
 *        of the head sequence.
 * 
 */						
#define SHIFTOUT_FAST		0x10	

/**
 * @brief Function to search a frame inside a circular buffer and automatically
 *        advance the buffer.
 * 
 * This function implements the exact same functionalities of searchFrame() by
 * passing it the same search_frame_rule rules structure, with additional 
 * features which allow to automatically advance the buffer, pulling bytes 
 * from the buffer head to free space for new incoming bytes pulled from the
 * tail.
 * 
 * SHIFTOUT FLAGS
 * The frame advance behavior can be configured by passing some flags, flags
 * can be or-ed with | to activate more than one:
 * - SHIFTOUT_NONE: no shift is performed, the stream buffer is untouched. This
 *   is overridden by all the other flags.
 * - SHIFTOUT_FULL: if no frame was found and the stream buffer is full,
 *   shift out a single byte.
 * - SHIFTOUT_CURR: if a frame was found, shift the stream buffer until the
 *   start of the found frame head (so the first frame of head will be at
 *   virtual index 0)
 * - SHIFTOUT_NEXT: if a frame was found, shift the stream buffer until the
 *   second byte of the found frame (equivalent to SHIFTOUT_CURR plus one
 *   additional byte), this ensures that at the next call of the function the
 *   old frame won't be found again. This overwrites SHIFTOUT_CURR and is the
 *   standard flag that should be used to ensure that a new frame is found at
 *   the next function call.
 * - SHIFTOUT_FOUND: if a frame was found, shift out the entire found frame
 *   from the stream buffer, this is an alternative to SHIFTOUT_NEXT in a 
 *   standard configuration, but can be risky in case it's used with soft
 *   policy since it could shift out valid head sequences happening to be into
 *   the currently found but invalid frame. This overwrites SHIFTOUT_NEXT.
 * - SHIFTOUT_FAST: (applied after all the other shifts) this flag can be used
 *   to speed-up the buffer advance especially if we expect an high level of 
 *   clutter in it, what it does is basically to advance the buffer to the next
 *   occurrence of the first byte of the head sequence and is useful to avoid 
 *   the slow single byte advance that happens if we use SHIFTOUT_FULL with a
 *   buffer full of garbage bytes or similar.  
 *   NB. if SHIFTOUT_FAST is set with SHIFTOUT_CURR, whenever a frame is found
 *   the SHIFTOUT_FAST is useless and the buffer will stay to the current found
 *   frame, it has instead effect whenever no frame is found.
 * 
 * RETURN DATA
 * The function returns 0 if no frame was found, !0 if a frame was found.
 * The function returns the eventually found frame the same way the
 * searchFrame() function does: the memory buffer of stream handle is assigned
 * to frame handle and the other fields are filled so that the handle buffer
 * only contains the found frame, again the frame handle doesn't need to be
 * initialized before. This approach has an additional risk related to the fact
 * that the stream buffer is advanced by pulling old data from it: the frame 
 * buffer can point to memory region that have been pulled out from stream and
 * so could be possibly overwitten by push operations on the latter; it's then
 * mandatory to copy the found frame elsewhere before performing any write
 * operation on the stream buffer, in any case push operations on stream should
 * stop when it's full to avoid losing bytes, unless specifically wanted (for
 * example if we want to read a frame sometimes and discard all the others).
 * 
 * @param stream circular buffer where to search the frame and which will be
 *               automatically advanced
 * @param frame output circular buffer where the found frame will be returned
 * @param rule set of rules to configure the search
 * @param shiftFlags flags to configure the advance functionality
 * @return uint8_t 0 if no frame was found, !0 otherwise
 */
uint8_t searchFrameAdvance(circular_buffer_handle* stream, circular_buffer_handle* frame, search_frame_rule * rule, uint8_t shiftFlags);

#endif