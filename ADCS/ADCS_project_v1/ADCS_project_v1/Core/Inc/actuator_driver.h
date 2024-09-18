#ifndef ACTUATOR_DRIVER_H
#define ACTUATOR_DRIVER_H
#include "adc.h"
#include "tim.h"
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include "constants.h"

//VARIABLES
extern const float Rsense[]; //Ohm //Rsense value for each motor driver
extern const float Aipropri; //Adimensionale //Mirror ratio of driver current mirror circuit
extern const float Rmagnetorquer[]; //Ohm
extern bool int_flag1,int_flag2;

//Structure
typedef struct{
	TIM_HandleTypeDef* htim;
	float Freq;
	uint32_t pwm_channel1;
	uint32_t pwm_channel2;
	float duty_cycle;
	bool dir;
}Actuator_struct;

//FUNCTIONS
/**
  * @brief Function to define handler of an actuator
  * @param act actuator handler
  * @param htim timer handler
  * @param pwm_channel1 htim timer channel 1
  * @param pwm_channel2 htim timer channel 2
  * @retval none
  */
void init_actuator_handler(Actuator_struct *act,TIM_HandleTypeDef* htim,uint32_t pwm_channel1,uint32_t pwm_channel2,uint32_t pwm_freq,uint8_t duty_cycle);

/**
  * @brief  Function to read current value of a selected actuator
  * @param hadc Handler for ADC1
  * @param currentbuf is the buffer that contains the current flowing trough all actuators
  * @retval none
  */
void get_actuator_current(ADC_HandleTypeDef *hadc,volatile float voltagebuf[],volatile float currentbuf[],uint8_t Channels_mask[]);
//void get_actuator_current(void)
/**
  * @brief  Function to update pwm freq on run-time
  * @param act actuator handler
  * @retval none
  */
void update_pwm_Frequency(Actuator_struct *act);  //DA IMPLEMENTARE
/**
  * @brief  Function to update either dutycyle or current direction or both
  * @param act actuator handler
  * @param change_dir boolean to set if you want to change current direction
  * @retval none
  */
void update_duty_dir(Actuator_struct *act,float duty,bool dir);
/**
  * @brief  Function to START PWM
  * @param act actuator handler
  * @retval none
  */
void actuator_START(Actuator_struct *act);
/**
  * @brief  Function to STOP PWM
  * @param act actuator handler
  * @retval none
  */
void actuator_STOP(Actuator_struct *act);


void ADC_Select_CH1 (ADC_HandleTypeDef *hadc);
void ADC_Select_CH2 (ADC_HandleTypeDef *hadc);
void ADC_Select_CH3 (ADC_HandleTypeDef *hadc);
void ADC_Select_CH4 (ADC_HandleTypeDef *hadc);
void ADC_Select_CH16 (ADC_HandleTypeDef *hadc);



void set_init_frequency(Actuator_struct *act,uint32_t pwm_freq);
#endif
