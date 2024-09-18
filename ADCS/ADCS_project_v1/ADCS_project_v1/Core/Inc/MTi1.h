#ifndef MTI1_H
#define MTI1_H

/* Function to manage a MTI-2 / MTI-3 imu
 * protocol description: https://www.xsens.com/hubfs/Downloads/Manuals/MT_Low-Level_Documentation.pdf
 *
 * only the limited subset of needed functionalieties has been implemented
 * */

#include "frameUtils.h"
#include "bufferUtils.h"
#include <stdint.h>
#include "UARTdriver.h"
#include <stddef.h>
#include "usart.h"

#define IMU_BUFFER_LEN 100	//local buffer length

/* Function to init IMU, it delays so it must be called when HAL_GetTick interrupts are enabled */
/* You should passs the IMU UART handle as argument*/
/* returns 1 in case of success, 0 otherwise (no ack received) */
uint8_t initIMUConfig(UART_HandleTypeDef* IMUhandle);

/* Wait and read an IMU packet*/
/* Ypu should pass the IMU UART handle, the buffers where to store the data and the read timeout*/
//returns 1 in case of success, 0 otherwise
uint8_t readIMUPacket(UART_HandleTypeDef* IMUhandle, float gyroscope[3], float magnetometer[3], uint32_t timeout);

#endif
