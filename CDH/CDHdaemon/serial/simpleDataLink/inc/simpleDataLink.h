/**
 * @file simpleDataLink.h
 * @author Simone Bollattino (simone.bollattino@gmail.com)
 * @brief Simple data link protocol functions
 * 
 */

#ifndef SIMPLEDATALINK_H
#define SIMPLEDATALINK_H

#include "bufferUtils.h"
#include "frameUtils.h"

/**
 * @brief Frame header format
 * 
 * Defined here only to allow using sizeof() for array dimensions
 * The user can be completely unaware of this struct
 * 
 */
typedef struct{
    uint8_t code; ///< frame code (FRMCODE_)
    uint8_t ackWanted; ///< flag to signal that the frame wants an ack
    uint16_t hash; ///< frame hash (for acknowledges)
}__attribute__((packed)) frameHeader;

/**
 * @brief Macro which defines the maximum payload length
 * 
 * This corresponds to the maximum length of only the frame payload
 * (frame header and CRC excluded).
 * 
 */
#define SDL_MAX_PAY_LEN 128

/**
 * @brief Macro which enables anti lock feature and defines its depth
 * 
 * This macro enables the anti-deadlock feature (sdlSend() will try
 * receiving non ack frames while waiting for an ack, placing them
 * inside a temporary queue, this macro defines the depth of this queue),
 * this enables avoiding deadlocks that can verify if both endpoints happen
 * to be waiting for an ack at the same time.
 * NB: this can become memory intensive since it will instantiate another
 * buffer of SDL_MAX_PAY_LEN * SDL_ANTILOCK_DEPTH bytes so enable only if
 * needed.
 */
#define SDL_ANTILOCK_DEPTH 5

/**
 * @brief Macro which enables the ____sdlTestSendCallback() function
 * 
 * If this is defined, the ____sdlTestSendCallback() function is enabled
 * to debug the library in a single thread.
 * 
 */
//#define SDL_DEBUG 

/**
 * @brief Struct containing transmission and reception functions of the serial
 *        line and reception buffer
 * 
 * This structure will be used as a handle function for the serial line, the
 * user should write TX and RX functions wrappers for the specific line used
 * the functions must be NON BLOCKING and have the following format:
 * 
 * byte argument: byte to be sent or pointer where to write received byte
 * return: 0 if the byte could not be sent, !0 otherwise
 * 
 * This approach makes the protocol higly portable and device independent.
 * 
 * The structure also contains settings about the serial line behavior:
 * the timeout to wait for ack in case of transmission of a frame that
 * wants it, the number of retries in case no ack was received and the
 * last received frame hash (used on reception side), a frame hash is 
 * assigned to each frame and it helps discarding a frame that was already 
 * received but the ack did not arrive at destination and so the transmission
 * side sent it again. 
 * 
 * The handle must be initialized with sdlInitLine(), this will assign the
 * function pointers and initialize the reception buffer, from that point
 * the user should never touch the handle members again but instead only use
 * sdlSend() and sdlReceive()
 */
typedef struct{
    uint8_t (*txFunc)(uint8_t byte); ///< TX function pointer
    uint8_t (*rxFunc)(uint8_t* byte); ///< RX function pointer
    circular_buffer_handle rxBuff;   ///< Rx buffer handle
    uint8_t rxBuffArray[(sizeof(frameHeader)+SDL_MAX_PAY_LEN+2)*2]; ///< Rx buffer memory array
    circular_buffer_handle tmpBuff; ///< Temporary buffer for frame
    uint8_t tmpBuffArray[(sizeof(frameHeader)+SDL_MAX_PAY_LEN+2)*2]; ///< Temporary buffer for frame array
    uint32_t timeout; ///< Serial line timeout value (same unit of sdlTimeTick())
    uint32_t retries; ///< Number of retries in case of ack not received
    uint16_t lastRxHash; ///< Last frame hash received
#ifdef SDL_ANTILOCK_DEPTH
    circular_buffer_handle alockBuff; ///< Anti lock buffer handle
    uint8_t alockBuffArray[SDL_ANTILOCK_DEPTH*SDL_MAX_PAY_LEN]; ///< Anti lock buffer array
    circular_buffer_handle alockQueue; ///< Queue for received frames length handle
    uint8_t alockQueueArray[SDL_ANTILOCK_DEPTH*sizeof(uint32_t)]; ///< Queue for received frames length array
    ///< TODO: sizeof(uint32_t) is unelegant, should be changed by defining a type for elemNum in bufferUtils.h
#endif
}serial_line_handle;

/**
 * @brief Get the current tick time (should be defined by user)
 * 
 * This function is used to compute the timeout and should be defined
 * by the user, it should return a tick counter with the same unit of
 * the timeout value of serial_line_handle.
 * 
 * @return uint32_t current tick counter value
 */
uint32_t sdlTimeTick();

/**
 * @brief Init serial line handle.
 * 
 * This function inits a serial line handle in order for it to be used with
 * sdlSend() and sdlReceive(), the function needs to receive the I/O functions
 * pointers as argument, the timeout period and number of retries.
 * NB: txFunc and rxFunc can also be NULL if the serial line should work only
 * on TX or RX mode, in that case sdlSend() and sdlReceive() simply won't work,
 * obviously in that case you won't be able to transmit frames which need an
 * acknowledge.
 * See serial_line_handle documentation above for the format needed by those
 * functions.
 * 
 * @param line serial line handle to be initialized
 * @param txFunc tx function pointer 
 * @param rxFunc rx function pointer
 * @param timeout serial line timeout period
 * @param retries number of retries the transmission will make if no ack receved
 *                (the first transmission is not counted, so 0 meaning a single
 *                transmission try)
 */
void sdlInitLine(serial_line_handle* line, uint8_t (*txFunc)(uint8_t byte), uint8_t (*rxFunc)(uint8_t* byte), uint32_t timeout, uint32_t retries);

/**
 * @brief Send payload through serial line
 * 
 * This is the top function for transmission with this simple data link layer.
 * The function needs a serial line handler, a buffer containing the payload
 * and the lenght of the latter.
 * NB: The function will return error if the length len is higher than the
 * maximum allowed payload length, defined as the SDL_MAX_PAY_LEN macro.
 * 
 * @param line serial line handle where to send
 * @param buff array containing the payload
 * @param len length of the payload (must be <= SDL_MAX_PAY_LEN)
 * @param ackWanted flag to signal if we want to receive an ack for this frame
 * @return uint8_t 0 in case of error, !0 otherwise
 */
uint8_t sdlSend(serial_line_handle* line, uint8_t* buff, uint32_t len, uint8_t ackWanted);

/**
 * @brief Receive payload from serial line
 * 
 * This is the top function for reception with this simple data link layer.
 * The function needs a serial line handler, a buffer where to write the
 * payload and the length of the latter.
 * NB: the len argument only represents the reception array dimension, the
 * actually received payload can be shorter and its length will be returned 
 * by the function. The len argument can have any value, also higher or lower
 * than SDL_MAX_PAY_LEN macro but it's recommended to at least provide a len
 * of SDL_MAX_PAY_LEN in order to not miss any payload.
 * The function will not return received payloads that are higher than
 * the len argument or SDL_MAX_PAY_LEN.
 * 
 * @param line serial line handle where to receive
 * @param buff array where the payload will be written
 * @param len length of the array
 * @return uint32_t length of the received payload, 0 if no payload or error
 */
uint32_t sdlReceive(serial_line_handle* line, uint8_t* buff, uint32_t len);


/**
 * @brief Callback called between transmission and ack wait
 * 
 * This callback should be used for testing purposes onlt, it's called
 * whenever a frame which requires an ack is transmitted and before
 * the wait for an acknowledge starts, this way the test can simulate
 * the receiver endpoint operation in a single thread context (otherwise
 * the sdlSend funcion would always fail to receive an acknowledge before
 * the timeout expires)
 * 
 * @param line the serial line handle which raised the callback
 * @return __weak 
 */
#ifdef SDL_DEBUG
void __sdlTestSendCallback(serial_line_handle* line);
#endif

#endif