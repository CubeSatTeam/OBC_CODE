/**
 * @file bufferUtils.h
 * @author Simone Bollattino (simone.bollattino@gmail.com)
 * @brief Implementation of buffers/FIFOs and relative utility functions.
 * 
 */

#ifndef BUFFERUTILS_H
#define BUFFERUTILS_H

#include <stdint.h>

/**
 * @brief Enable buffer print funxtions (needs printf() to be available)
 */
#define BUFFERUTILS_ENABLEPRINT 

#ifdef BUFFERUTILS_ENABLEPRINT
#include <stdio.h>
#endif

/**
 * @brief Flag to print buffer in default way (virtual index ordering)
 */
#define PRINTBUFF_DEFAULT 0x00
/**
 * @brief flag to print buffer with memory index order (default is virtual)
 */
#define PRINTBUFF_MEMORY 0x01
/**
 * @brief flag to print buffer with also metadata
 */
#define PRINTBUFF_METADATA 0x02
/**
 * @brief flag to avoid putting newline at the end of buffer print
 */
#define PRINTBUFF_NONEWLINE 0x04
/**
 * @brief flag to avoid printing buffer bytes that are outside the buffer
 */
#define PRINTBUFF_NOEMPTY 0x08
/**
 * @brief flag to print data in hexadecimal formt
 */
#define PRINTBUFF_HEX 0x10

//PLAIN BUFFER UTILITIES ----------------------------------

/**
 * @brief Plain buffer handle struct.
 * 
 * This is basically a normal array buffer with added informations on size
 * and number of elements.
 */
typedef struct{
	uint8_t * buff;			///< buffer on memory
	uint32_t buffLen; 		///< length of memory allocation (buffer length)
	uint32_t elemNum;		///< number of elements inside buffer
} plain_buffer_handle;

#ifdef BUFFERUTILS_ENABLEPRINT 
/**
 * @brief Print plain buffer in human readable way.
 * 
 * By passing flag PRINTBUFF_DEFAULT, the function will print the buffer
 * with virtual index ordering and without metadata, the function can be
 * configured to print also print buffer metadata by passing the flag 
 * PRINTBUFF_METADATA, PRINTBUFF_MEMORY flag makes the function print the
 * buffer with memory index ordering, which in this case is the same as
 * virtual index ordering so this flag is equivalent to PRINTBUFF_DEFAULT.
 * The PRINTBUFF_NONEWLINE flag can be passed to avoid putting a newline
 * character at the end of the buffer print.
 * The PRINTBUFF_NOEMPTY flag can be passed to avoid printing buffer memory
 * indexes that are outside the virtual indexes range (otherwise they will
 * be printed as "__").
 * 
 * @param handle buffer handle
 * @param flags print configuration flags
 */
void pBuffPrint(plain_buffer_handle* handle, uint8_t flags);
#endif

/**
 * @brief Init a plain buffer handle.
 * 
 * Can also be used to reset it, but pBuffFlush() is to be preferred instead.
 * 
 * @param handle buffer handle
 * @param buff memory array
 * @param buffLen length of memory array
 * @param elemNum number of valid elements if array was already populated
 */
void pBuffInit(plain_buffer_handle* handle, uint8_t* buff, uint32_t buffLen, uint32_t elemNum);

/**
 * @brief Write data to plain buffer overwriting elements already present in it.
 * 
 * Write can be done starting from buffer head or tail and with an additional 
 * offset from that position.
 * Write will occurr for a certain amount of bytes or until the other end of the 
 * buffer is reached (even if an offset is provided, writing will eventually stop 
 * at the head or tail without circularity), if the offset provided is outside the
 * buffer boundaries nothing will be written.
 * The data will always be written respecting the order of the input data (if write
 * is done starting from tail going backwards at the end the written data will have
 * its order reversed on the buffer).
 * The function also accepts a NULL data buffer, in that case nothing will be written
 * but it will still return the number of bytes that it could have written.
 * 
 * @param handle buffer handle
 * @param data data buffer to write
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off starting offset from head or tail (depending on ht)
 * @return uint32_t the actual number of written bytes
 */

uint32_t pBuffWrite(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off);

/**
 * @brief Read data from plain buffer from elements already present in it.
 * 
 * Read can be done starting from buffer head or tail and with an additional 
 * offset from that position.
 * Read will occurr for a certain amount of bytes or until the other end of the 
 * buffer is reached (even if an offset is provided, reading will eventually stop 
 * at the head or tail without circularity), if the offset provided is outside the
 * buffer boundaries nothing will be read.
 * The data will always be read respecting the order of the input data (if read
 * is done starting from tail going backwards at the end the read data will have
 * its order reversed on the output array).
 * The function also accepts a NULL data buffer, in that case nothing will be read
 * but it will still return the number of bytes that it could have read.
 * 
 * @param handle buffer handle
 * @param data output data buffer to read into
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off starting offset from head or tail (depending on ht)
 * @return uint32_t the actual number of read bytes
 */
uint32_t pBuffRead(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off);

/**
 * @brief Write single byte inside plain buffer.
 * 
 * Write can be done at an offset starting from head or tail.
 * If an offset outside the buffer boundaries (elements number) is given, nothing
 * will be written.
 * 
 * @param handle buffer handle
 * @param val value to write
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off offset from head or tail (depending on ht)
 */
void pBuffWriteByte(plain_buffer_handle* handle, uint8_t val, uint8_t ht, uint32_t off);

/**
 * @brief Read single byte from plain buffer.
 * 
 * Read can be done at an offset starting from head or tail.
 * If an offset outside the buffer boundaries (elements number) is given, it will
 * return 0
 * 
 * @param handle buffer handle
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off offset from head or tail (depending on ht)
 * @return uint8_t byte value, 0 if offset outside boundaries
 */
uint8_t pBuffReadByte(plain_buffer_handle* handle, uint8_t ht, uint32_t off);

/**
 * @brief Push data on plain buffer.
 * 
 * Data can be pushed starting from head or tail, if data is pushed from head
 * the memory array will be shifted so that at the end the head element is 
 * always at physical index 0 (not computationally friendly), for this reason
 * the use of plain buffers for this purpose is discouraged in favor of 
 * circular ones.
 * Pushing stops if the buffer gets full.
 * 
 * @param handle buffer handle
 * @param data data buffer to push
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 * @return uint32_t the actual number of pushed bytes
 */
uint32_t pBuffPush(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht);

/**
 * @brief Pull data from plain buffer.
 * 
 * Data can be pulled starting from head or tail, if data is pulled from head
 * the memory array will be shifted so that at the end the head element is at
 * physical index 0 (not computationally friendly), for this reason
 * the use of plain buffers for this purpose is discouraged in favor of 
 * circular ones.
 * Pulling stops if the buffer gets empty.
 * 
 * @param handle buffer handle
 * @param data output data buffer to pull into
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 * @return uint32_t the actual number of pulled bytes
 */
uint32_t pBuffPull(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht);

/**
 * @brief Pull data from plain buffer starting from a middle point.
 * 
 * This function can be used to pull bytes from the buffer by starting from
 * a point in the middle (offset from head/tail), the hole created will be 
 * closed by shifting the remaining bytes of the buffer (can be intensive).
 * Cut will occurr for a certain amount of bytes or until the other end of the 
 * buffer is reached (even if an offset is provided, cutting will eventually stop 
 * at the head or tail without circularity), if the offset provided is outside the
 * buffer boundaries nothing will be read.
 * The data will always be read respecting the order of the input data (if cut
 * is done starting from tail going backwards at the end the read data will have
 * its order reversed on the output array).
 * The function also accepts a NULL data buffer, in that case nothing will be read
 * but it will still cut the data from buffer and return the number of cutted bytes.
 * 
 * @param handle buffer handle
 * @param data output data buffer to read into
 * @param dataLen length of data buffer (length of cutted portion)
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off starting offset from head or tail (depending on ht)
 * @return uint32_t the actual number of cutted bytes
 */
uint32_t pBuffCut(plain_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off);

/**
 * @brief Flush plain buffer.
 * 
 * @param handle buffer handle
 */
void pBuffFlush(plain_buffer_handle* handle);

/**
 * @brief Check if plain buffer is full.
 * 
 * @param handle buffer handle
 * @return uint8_t 0 if buffer is NOT full, !0 otherwise
 */
uint8_t pBuffFull(plain_buffer_handle* handle);

/**
 * @brief Check if plain buffer is empty.
 * 
 * @param handle buffer handle
 * @return uint8_t 0 if buffer is NOT empty, !0 otherwise
 */
uint8_t pBuffEmpty(plain_buffer_handle* handle);

//CIRCULAR BUFFER UTILITIES -------------------------------

/**
 * @brief Circular buffer handle struct.
 * 
 * This is basically a normal array buffer with added informations on size
 * and number of elements.
 */
typedef struct{
	uint8_t * buff;			///< buffer on memory
	uint32_t buffLen; 		///< length of memory allocation (buffer length)
	uint32_t elemNum;		///< number of elements inside buffer
	uint32_t startIndex;	///< starting index of buffer
} circular_buffer_handle;

#ifdef BUFFERUTILS_ENABLEPRINT
/**
 * @brief Print plain buffer in human readable way.
 * 
 * By passing flag PRINTBUFF_DEFAULT, the function will print the buffer
 * with virtual index ordering and without metadata, the function can be
 * configured to print also print buffer metadata by passing the flag 
 * PRINTBUFF_METADATA, PRINTBUFF_MEMORY flag makes the function print the
 * buffer with memory index ordering.
 * The PRINTBUFF_NONEWLINE flag can be passed to avoid putting a newline
 * character at the end of the buffer print.
 * The PRINTBUFF_NOEMPTY flag can be passed to avoid printing buffer memory
 * indexes that are outside the virtual indexes range (otherwise they will
 * be printed as "__").
 *
 * @param handle buffer handle
 * @param flags print configuration flags
 */
void cBuffPrint(circular_buffer_handle* handle, uint8_t flags);
#endif

/**
 * @brief Init a circular buffer handle.
 * 
 * Can also be used to reset it, but pBuffFlush() is to be preferred instead.
 * 
 * @param handle buffer handle
 * @param buff memory array
 * @param buffLen length of memory array
 * @param elemNum number of valid elements if array was already populated
 */
void cBuffInit(circular_buffer_handle* handle, uint8_t* buff, uint32_t buffLen, uint32_t elemNum);

/**
 * @brief Get memory (physical) index corresponding to virtual index of circular buffer.
 * 
 * The function will return a memory index also if the given virtual index is
 * outside the buffer boundaries (not containing valid elements).
 * This function can be used to get the circular buffer starting index value
 * (head position inside memory array).
 * 
 * @param handle buffer handle
 * @param virtIndex virtual index
 * @return uint32_t corresponding memory index
 */
uint32_t cBuffGetMemIndex(circular_buffer_handle* handle,uint32_t virtIndex);

/**
 * @brief Get virtual index corresponding to memory (physical) index of circular buffer.
 * 
 * The function will return a virtual index also if the given memory index is
 * outside the buffer boundaries (not containing valid elements).
 * 
 * @param handle buffer handle
 * @param memIndex memory index
 * @return uint32_t corresponding virtual index
 */
uint32_t cBuffGetVirtIndex(circular_buffer_handle* handle,uint32_t memIndex);

 /**
 * @brief Write data to circular buffer overwriting elements already present in it.
 * 
 * Write can be done starting from buffer head or tail and with an additional 
 * offset from that position.
 * Write will occurr for a certain amount of bytes or until the other end of the 
 * buffer is reached (even if an offset is provided, writing will eventually stop 
 * at the head or tail without circularity), if the offset provided is outside the
 * buffer boundaries nothing will be written.
 * The data will always be written respecting the order of the input data (if write
 * is done starting from tail going backwards at the end the written data will have
 * its order reversed on the buffer).
 * The function also accepts a NULL data buffer, in that case nothing will be written
 * but it will still return the number of bytes that it could have written.
 * 
 * @param handle buffer handle
 * @param data data buffer to write
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off starting offset from head or tail (depending on ht)
 * @return uint32_t the actual number of written bytes
 */
uint32_t cBuffWrite(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off);

/**
 * @brief Read data from circular buffer from elements already present in it.
 * 
 * Read can be done starting from buffer head or tail and with an additional 
 * offset from that position.
 * Read will occurr for a certain amount of bytes or until the other end of the 
 * buffer is reached (even if an offset is provided, reading will eventually stop 
 * at the head or tail without circularity), if the offset provided is outside the
 * buffer boundaries nothing will be read.
 * The data will always be read respecting the order of the input data (if read
 * is done starting from tail going backwards at the end the read data will have
 * its order reversed on the output array).
 * The function also accepts a NULL data buffer, in that case nothing will be read
 * but it will still return the number of bytes that it could have read.
 * 
 * @param handle buffer handle
 * @param data output data buffer to read into
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off starting offset from head or tail (depending on ht)
 * @return uint32_t the actual number of read bytes
 */
uint32_t cBuffRead(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off);

/**
 * @brief Write single byte inside circular buffer.
 * 
 * Write can be done at an offset (virtual) starting from head or tail.
 * If an offset outside the buffer boundaries (elements number) is given, nothing
 * will be written.
 * 
 * @param handle buffer handle
 * @param val value to write
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off offset (virtual) from head or tail (depending on ht)
 */
void cBuffWriteByte(circular_buffer_handle* handle, uint8_t val, uint8_t ht, uint32_t off);

/**
 * @brief Read single byte from circular buffer.
 * 
 * Read can be done at an offset (virtual) starting from head or tail.
 * If an offset outside the buffer boundaries (elements number) is given, it will
 * return 0.
 * 
 * @param handle buffer handle
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off offset (virtual) from head or tail (depending on ht)
 * @return uint8_t byte value, 0 if offset outside boundaries
 */
uint8_t cBuffReadByte(circular_buffer_handle* handle, uint8_t ht, uint32_t off);

/**
 * @brief Push data on circular buffer.
 * 
 * Data can be pushed starting from head or tail, the push operation doesn't involve
 * a shift of the emory array and for this reason it's more lightweight than using
 * plain buffers for such applications.
 * If the buffer gets full, elements on the other end of the buffer will be
 * overwritten with the new data.
 * 
 * @param handle buffer handle
 * @param data data buffer to push
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 */
void cBuffPush(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht);

/**
 * @brief Push data on circular buffer by stopping if it gets full.
 * 
 * Wrapper of cBuffPush() which stops if the buffer gets full without
 * overwriting on the other side circularlt.
 * The actual number of pushed bytes are returned by the function.
 * 
 * @param handle buffer handle
 * @param data data buffer to push
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 * @return uint32_t the actual number of pushed bytes
 */
uint32_t cBuffPushToFill(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht);

/**
 * @brief Pull data from circular buffer.
 * 
 * Data can be pulled starting from head or tail, the pull operation doesn't involve
 * a shift of the emory array and for this reason it's more lightweight than using
 * plain buffers for such applications.
 * Pulling stops if the buffer gets empty.
 * 
 * @param handle buffer handle
 * @param data output data buffer to pull into
 * @param dataLen length of data buffer
 * @param ht start from head or tail flag (0=head !0=tail)
 * @return uint32_t the actual number of pulled bytes
 */
uint32_t cBuffPull(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht);

/**
 * @brief Pull data from circuar buffer starting from a middle point.
 * 
 * This function can be used to pull bytes from the buffer by starting from
 * a point in the middle (offset from head/tail), the hole created will be 
 * closed by shifting the remaining bytes of the buffer (can be intensive).
 * Cut will occurr for a certain amount of bytes or until the other end of the 
 * buffer is reached (even if an offset is provided, cutting will eventually stop 
 * at the head or tail without circularity), if the offset provided is outside the
 * buffer boundaries nothing will be read.
 * The data will always be read respecting the order of the input data (if cut
 * is done starting from tail going backwards at the end the read data will have
 * its order reversed on the output array).
 * The function also accepts a NULL data buffer, in that case nothing will be read
 * but it will still cut the data from buffer and return the number of cutted bytes.
 * 
 * @param handle buffer handle
 * @param data output data buffer to read into
 * @param dataLen length of data buffer (length of cutted portion)
 * @param ht start from head or tail flag (0=head !0=tail)
 * @param off starting offset from head or tail (depending on ht)
 * @return uint32_t the actual number of cutted bytes
 */
uint32_t cBuffCut(circular_buffer_handle* handle, uint8_t* data, uint32_t dataLen, uint8_t ht, uint32_t off);

/**
 * @brief Function to pull data from a circular buffer and push it into another.
 * 
 * This function can be used to move data between one buffer to another.
 * The data can be pushed or pulled from any combination of head and tail of both
 * buffers and for a certain amout of bytes. The function will return the actual
 * number of moved bytes.
 * 
 * @param dest destination circular buffer
 * @param source source circular buffer
 * @param len number of bytes to move
 * @param htDest flag to signal if push should happen from head or tail
 * @param htSource flag to signal if pull should happen from head or tail
 * @return uint32_t the actual number of moved bytes
 */
uint32_t cBuffPushPull(circular_buffer_handle* dest, circular_buffer_handle* source, uint32_t len, uint8_t htDest, uint8_t htSource);

/**
 * @brief Function to read data from a circular buffer and push it into another.
 * 
 * This function can be used to move data between one buffer to another, without
 * pushing out the data from the source.
 * The data can be pushed or read from any combination of head and tail of both
 * buffers and for a certain amout of bytes. The function will return the actual
 * number of moved bytes.
 * 
 * @param dest destination circular buffer
 * @param source source circular buffer
 * @param len number of bytes to move
 * @param htDest flag to signal if push should happen from head or tail
 * @param htSource flag to signal if read should happen from head or tail
 * @return uint32_t the actual number of moved bytes
 */
uint32_t cBuffPushRead(circular_buffer_handle* dest, circular_buffer_handle* source, uint32_t len, uint8_t htDest, uint8_t htSource);

/**
 * @brief Rotate circular buffer
 * 
 * Rotation can be performed clockwise or anti-clockwise and is performed on data
 * so it's equivalent of moving the start position on the opposite direction.
 * Rotation is performed by only moving the minimum number of bytes in memory 
 * to preserve buffer integrity but this involves shifting some buffer elements
 * (not computationally friendly)
 * 
 * @param handle buffer handle
 * @param dir data rotation direction (0=anti-clockwise !0=clockwise)
 * @param pos number of position to rotate
 */
void cBuffRotate(circular_buffer_handle* handle, uint8_t dir, uint32_t pos);

/**
 * @brief Flush circular buffer.
 * 
 * @param handle buffer handle
 */
void cBuffFlush(circular_buffer_handle* handle);

/**
 * @brief Check if circular buffer is full.
 * 
 * @param handle buffer handle
 * @return uint8_t 0 if buffer is NOT full, !0 otherwise
 */
uint8_t cBuffFull(circular_buffer_handle* handle);

/**
 * @brief Check if circular buffer is empty.
 * 
 * @param handle buffer handle
 * @return uint8_t 0 if buffer is NOT empty, !0 otherwise
 */
uint8_t cBuffEmpty(circular_buffer_handle* handle);

/**
 * @brief Convert circular buffer to plain buffer
 * 
 * A memory rotation is performed on the circular buffer to have it startIndex at 0
 * NB: The memory array is assigned to the pBuffer so at the end they will SHARE the
 * memory region. The plain buffer doesn't need to be initialized before.
 * 
 * @param pHandle plain buffer handle (destination)
 * @param cHandle circular buffer handle (source)
 */
void cBuffToPlain(plain_buffer_handle* pHandle, circular_buffer_handle* cHandle);

/**
 * @brief Convert plain buffer to circular buffer
 * 
 * NB: The memory array is assigned to the pBuffer so at the end they will SHARE the
 * memory region. The circular buffer doesn't need to be initialized before.
 *
 * @param cHandle circular buffer handle (destination)  
 * @param pHandle plain buffer handle (source) 
 */
void pBuffToCirc(circular_buffer_handle* cHandle, plain_buffer_handle* pHandle);

/**
 * @brief Copy plain buffer to another plain buffer
 * 
 * This function basically creates a copy of a plain buffer into another.
 * NB: The memory array is assigned to the destination so at the end they 
 * will SHARE the memory region. The dest buffer doesn't need to be initialized
 * before.
 *
 * @param dest destination plain buffer
 * @param pHandle source plain buffer
 */
void pBuffToPlain(plain_buffer_handle* dest, plain_buffer_handle* pHandle);

/**
 * @brief Copy circular buffer to another circular buffer
 * 
 * This function basically creates a copy of a circular buffer into another.
 * NB: The memory array is assigned to the destination so at the end they 
 * will SHARE the memory region. The dest buffer doesn't need to be initialized
 * before.
 *
 * @param dest destination circular buffer
 * @param pHandle source circular buffer
 */
void cBuffToCirc(circular_buffer_handle* dest, circular_buffer_handle* cHandle);

#endif