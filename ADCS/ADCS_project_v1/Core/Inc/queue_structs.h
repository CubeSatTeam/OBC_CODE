/*
 * queue_struct.h
 *
 *  Created on: 8 mag 2024
 *      Author: Utente
 */

#ifndef INC_QUEUE_STRUCTS_H_
#define INC_QUEUE_STRUCTS_H_

#include "sensors.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "cmsis_os.h"
#include "pid_conversions.h"
#include <stdio.h>
#include <malloc.h>
#include "messages.h"
#include "constants.h"

//Structures
typedef struct{
	float gyro_msr[3];
	float mag_msr[3];
	float acc_msr[3];
} imu_queue_struct;

typedef struct{
	float current[NUM_ACTUATORS];
	float temperature[NUM_TEMP_SENS];
} Current_Temp_Struct;

//Functions
/**
  * @brief  Callback function to process data
  * @param	event First pointer to void variable
  * @param	data1 Second pointer to void variable
  * @retval none
  */
typedef void (*CombinedDataProcessor)(void *event,void *data1);

/**
  * @brief  Function to handle pointers to void and a pointer to void struct function
  * @param	event First pointer to void variable
  * @param	strct1 Second pointer to void variable
  * @param 	processor Typedef struct function parameter
  * @retval none
  */
void processCombinedData(void *event,void *strct1, CombinedDataProcessor processor);

/**
  * @brief  Function to handle received IMU queue elements(sent by IMU Task) arriving to Control Task
  * @param	event First pointer to void variable
  * @param	PID_struct Second pointer to void variable
  * @retval none
  */
void receive_IMUqueue_control(void *event,void *PID_struct);
/**
  * @brief  Function to handle received IMU queue elements(sent by IMU Task) arriving to OBC Task
  * @param	event First pointer to void variable
  * @param	attitude Second pointer to void variable
  * @retval none
  */
void receive_IMUqueue_OBC(void *event,void *attitude);
/**
  * @brief  Function to handle received Temperatures and currents queue(sent by Check Task) elements arriving to OBC Task
  * @param	event First pointer to void variable
  * @param	current_temp_struct Second pointer to void variable
  * @retval none
  */
void receive_Current_Tempqueue_OBC(void *event,void *current_temp_struct);
/**
  * @brief  
  * @param	event First pointer to void variable
  * @param	attitude_adcs Second pointer to void variable
  * @retval none
  */
void receive_Attitudequeue_control(void *event,void *attitude_adcs);

#endif /* INC_QUEUE_STRUCTS_H_ */
