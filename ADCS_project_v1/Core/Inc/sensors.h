#ifndef SENSORS_H
#define SENSORS_H
//This is the header file of the library that handles temperature sensors,solar sensors and imu 
//on the ADCS board
#include "spi.h"
#include "UARTdriver.h"
#include <math.h>
#include <stdio.h> //for printf()
#include "constants.h"


extern uint8_t single_mode_pckt[2];
extern uint8_t WRITE_ON_MR; //The first byte to send on DIN to CR to start Conversation in Single MODE
extern uint8_t MR_FOR_SINGLE_MOD; //The second byte to send on DIN to start Conversation in Single MODE
extern uint8_t READ_DATAREG; //The byte to send on DIN to start obtain the result of Conversation on Dout
extern uint8_t READ_STATUSREG; //The byte to send on DIN to the CR to obtain the content of Status Reg
//STRUCTURES
//Struct per la memorizzazione dei valori di temperatura degli ntc

typedef struct{
  float R_25;
  float R[8];
  float B;
  float Vdd;
}T_formula_const;

typedef struct{
  float temp[8];
  T_formula_const values;

}Temp_values;
//FUNCTIONS
/**
  * @brief  Function to initialize all structure and variables needed for management of ADC(internal and external)
  * @param	Temp values Struct to handle all variables regarding temperature sensors
  * @retval none
  */
void init_tempsens_handler(Temp_values *Temp_values);

/**
  * @brief  Function to select which channel of analogic mux send to external ADC
  * @param	sel Variable to select channel(0 to 7)
  * @retval none
  */
void select_input(uint8_t sel);

/**
  * @brief  Function to make analog-to-digital conversion with external ADC
  * @param	spi_struct Structure needed to handle SPI communication with external ADC
  * @param  mode variable to select the working mode of ADC:
  * 1) mode = 0 : Continuous conversione mode
  * 2) mode = 1 : Single conversion mode
  * 3) mode = 2 : Continuous Read mode(Da implementare)
  * IF mode different from one of these values print an error
  * @param status_flag a flag to check the status of conversion
  * @retval none
  */
float ADC_Conversion(SPI_HandleTypeDef *spi_struct,uint8_t mode);
//uint16_t ADC_Conversion(SPI_HandleTypeDef *spi_struct,uint8_t mode);


/**
  * @brief  Function to select which channel of analogic mux send to external ADC
  * @param	sel Variable to select channel(0 to 7)
  * @retval none
  */
void voltage_to_temperature_conv(float value,Temp_values *s1,uint8_t i);


void get_temperatures(SPI_HandleTypeDef *spi_struct,Temp_values *temp_struct, uint8_t counter);

#endif
