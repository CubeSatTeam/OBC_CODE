#ifndef PID_CONVERSIONS_H
#define PID_CONVERSIONS_H
//This is the header file of the library that handles the variables and structures used in the PID controller
//on the ADCS board
#include <stdint.h>
#include "constants.h"


//#DEFINES

//no defines, update the main struct

//STRUCTURES
//Struct per la memorizzazione dei valori di temperatura degli ntc

typedef struct{


  uint8_t timestamp;
  float N_spires[3]; 			//TODO be set Initially		Static vector 	Number of spires of each magnetic torquer
  float A_torquers[3];			//TODO be set Initially		Static vector 	Area of each magnetic torquer
  float torquer_Req_Ohm[3];		//TODO be set Initially	Static vector 	Equivalent resistance of each magnetic torquer
  float torquer_Vdd[3];			//TODO be set Initially	Static vector	Volts of maximum tension for each magnetic torquer

  float accell_Measured[3];		//TODO be updated at each IMU reading		Dynamic vector  	IMU acceleration
  float accell_Desired[3];		//TODO be updated at each OBC reading		Dynamic vector  	Wanted OBC acceleration
  float accell_Error[6];		//Calculated Acceleration Error between measured and wanted
  float d_Accell_Err_dt[3];		//Calculated Derivative of the acceleration Error


  float angSpeed_Measured[3];		//TODO be updated at each IMU reading		Dynamic vector  	IMU acceleration
  float angSpeed_Desired[3];		//TODO be updated at each OBC reading		Dynamic vector  	Wanted OBC angular speed
  float angSpeed_Error[6];			//Calculated Angle speed Error between measured and wanted
  float d_AngSpeed_Err_dt[3];		//Calculated Derivative of the angle speed Error

  float current_Measured[3];		//TODO be updated at each Current reading		Dynamic vector  	Current sensor reading
  float current_Desired[3];			//TODO be updated at each OBC reading		Dynamic vector  	Wanted OBC current
  float curruent_Error[6];			//Calculated Current Error between measured and wanted
  float d_Current_Err_dt[3];		//Calculated Derivative of the current Error

  



  float Torque_required[3];  //Torque required from PID controller for velocity and speed errors

  float dipole_Moment[3];   //Dipole moment required from PID controller
  float th_Current[3]; 		//Current required for the PID controller
  float th_Dutycycle[3];    //Duty cycle required from PID controller

  float P_Gain[3];  //TBD
  float D_Gain[3];  //TBD
  
  float P_Current_Gain[3];  //TBD
  float D_Current_Gain[3];  //TBD

  float B[3];  //TODO be updated at each Current reading		Dynamic vector  	Current sensor reading



} PID_Inputs_struct;



//FUNCTIONS
void PID_error_calculation(PID_Inputs_struct *PID_Inputs);
/**
  * @brief  Function to calculate the error between two attitude measurements
  * @param	Attitude values Struct to handle all variables regarding attitude control
  * @retval none
  */

void PID_attitude_Derivative_calculation(PID_Inputs_struct *PID_Inputs);
/**
  * @brief  Function to calculate the derivative of the error to give in INPUT to the PID controller
  * @param	Attitude measurements and timestamp into Struct to handle all variables regarding attitude control
  * @retval none
  */


void PID_error_2_torque(PID_Inputs_struct *PID_INPUTS);
/**
  * @brief  Function to initialize all structure and variables needed for management of ADC(internal and external)
  * @param	Temp values Struct to handle all variables regarding temperature sensors
  * @retval none
  */


void PID_torque_2_dipole(PID_Inputs_struct *PID_Inputs);
/**
  * @brief  Function to initialize all structure and variables needed for management of ADC(internal and external)
  * @param	Temp values Struct to handle all variables regarding temperature sensors
  * @retval none
  */

void PID_dipole_2_current(PID_Inputs_struct *PID_INPUTS);
/**
  * @brief  Function to initialize all structure and variables needed for management of ADC(internal and external)
  * @param	Temp values Struct to handle all variables regarding temperature sensors
  * @retval none
  */

void PID_current_2_DutyCycle(PID_Inputs_struct *PID_Inputs);
/**
  * @brief  Function to initialize all structure and variables needed for management of ADC(internal and external)
  * @param	Temp values Struct to handle all variables regarding temperature sensors
  * @retval none
  */

  
void PID_INIT(PID_Inputs_struct *PID_Inputs);
/**
  * @brief  Function to initialize all structure and variables needed for management of ADC(internal and external)
  * @param	Temp values Struct to handle all variables regarding temperature sensors
  * @retval none
  */


void PID_main(PID_Inputs_struct *PID_Inputs);
/**
  * @brief  Function to initialize all structure and variables needed for management of ADC(internal and external)
  * @param	Temp values Struct to handle all variables regarding temperature sensors
  * @retval none
  */

#endif
