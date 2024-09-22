# Simple Data Link protocol - simpleDataLink.h/.c
This library implement a simple unreliable or reliable data link layer to be used with serial lines.
The concept behind the library is basically to take a payload buffer from the user, create a frame from that and send it on a generic serial line, eventually waiting for an ack if desired, with the possibility of re-trying to send it if the ack dodn't arrive.
The user will need to implement I/O functions towards the hardware (for example an UART), this allows porting the library easily to any architecture.

## Dependencies
This library depends on my bufferUtils (https://github.com/shimo97/bufferUtils) and frameUtils (https://github.com/shimo97/frameUtils) libraries, which are inserted as submodules on the git repo.

## Frame format
The frames are very simple and with a minimal header part, the frame format is the following:

| 0x7E | HEADER | PAYLOAD | CRC16 | 0x7E |

The frame is enclosed between two 0x7E flag bytes and contains the header, the payload plus a 16 bit CRC.

### Header
The header is composed of three fields:
| Field | Parallelism | Description |
| --- | --- | --- |
| code | 1 byte | Frame code (for now DATA or ACK) |
| ackWanted | 1 byte | Flag to signal that this frame wants an acknowledge as response |
| hash | 2 bytes | Hash to (possibly) uniquely identify a frame, so that it can be discarded if the preceding ack was lost and the other end resent it |

Right now, the hash is a simple 16 bit counter, which is incremented for every new frame, in the future it can be replaced with a more robust hash.

## Payload
The payload can have a maximum length of SDL_MAX_PAY_LEN.
NB:Network order is ensured ONLY for the header fields and CRC, the user needs to implement network ordering on the payload if needed.

## CRC-16
By default, the CRC-16 is implemented by using polynomial 0x1021 and initialization value 0xFFFF and uses a pre-generated look-up table to increase performance, the user can use another polynomial by generating a new look-up table with the printCRCLUT() function which is commented out in simpleDataLink.c, the LUT can be tested against the classic shift-xor algorithm by using the computeCRC() function which is also commented out on the same file.

## Byte Stuffing
The library implements an HDLC-like byte stuffing algorithm in order to eliminate any occurrence of 0x7E inside the payoad or the CRC, the algorithm works by adding an escape byte 0x7D before any occurrence of the flag byte 0x7E or the escape byte itself, the latter gets its 5-th byte inverted, like in the following table:

|Occurrence|Replaced by|
|---|---|
|0x7E| 0x7D 0x5E |
|0x7D| 0x7D 0x5D |

Byte stuffing allows an easy search of frames since it allows to have the 0x7E flag only at the begin/end of frames.

## Serial line handle and I/O functions
A serial line is represented by a serial_line_handle structure, this needs to be initialized with the sdlInitLine() function, this function needs two function pointers which point to I/O functions defined by the user, those functions will implement the transmission/reception of a single byte on the specifi serial line hardware (see simpleDataLink.h for more informations), allowing the library to be ported or used with different types of lines and drivers. The function also wants the desired timeout for the line and the number of retries in case of lost ack.

### Timeout
To be able to implement the timeout, the library also needs the user to define the sdlTimeTick() function to return a tick counter, the timeout given to sdlInitLine() will have the same unit of this counter.

## Frame send/receive functions
Finally, the library can be used with sdlSend() and sdlReceive() functions, those will handle everything, from frame creation/extraction to CRC creation/verification, byte stuffing, I/O on the line and acknowledges.

### sdlReceive()
This is the function which tries to receive a frame from serial line and eventually sends back an acknowledge if requested.
Since there's the possibility of a correctly received frame whose ack is lost (and a consequent retry to send the same frame from the other endpoint), this function will save the hash of the last correctly acknowledged frame inside the line handle structure and discard new frames having the same hash.
Received frames can be discarded also if the ack was sent in case the buffer given to sdlReceive() is too small, to avoid such case, you should always pass a buffer at least SDL_MAX_PAY_LEN long.

### sdlSend()
This is the function used to send frames and eventually wait for an ack, in the latter case the function is BLOCKING for the timeout given during serial line creation (multiplied by the number of retries). This can potentially lead to deadlocks: if both endpoints call this function at the same time, both would wait for an ack from sdlReceive() until timeout. To avoid this, an anti-deadlock feature has been added: the function basically tries to receive (and ack) frames while waiting for an acknowledge itself, inserting the eventually received frames inside a queue inside the serial line handle, sdlReceive() will then read from the queue at the next call or if the latter is empty, try to receve frames from the line. To enable this feature the SDL_ANTILOCK_DEPTH should be defined with the desired queue length (this can be memory consuming since it will instantiate an additional buffer of SDL_MAX_PAY_LEN * SDL_ANTILOCK_DEPTH bytes, so use with caution).

## Example
An example of usage of the library is provided in examples/communicationExample.c, in this program various tests are performed simulating different scenarios, to allow testing the library acknowledges, a test callback __sdlTestSendCallback() can be enabled by defining SDL_DEBUG macro, this callback should be defined by the user and is called inside the sdlSend() loop to allow simulating the other endpoint actions. 

## compile instructions
The source code can be compiled into a static library (simpleDataLink.a) by running **make** on the root directory, by default this will compile all sources into object files on the **build** folder and then pack them in the static library, to compile the example you can instead call **make example**, again this will compile the example executable on the **build** folder.
You can change the build folder or compiler flags (default **-Wall**) by passing variables to make like **make builddir=newbuilddirectory compflags=newcompilerflags**