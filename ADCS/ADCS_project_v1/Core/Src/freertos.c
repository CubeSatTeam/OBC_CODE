/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h> //for printf()
#include <string.h> //for memcpy()
#include <stdbool.h>
#include <math.h>
#include "usart.h"
#include "sensors.h"
#include "pid_conversions.h"
#include "UARTdriver.h"
#include "spi.h"
#include "adc.h"
#include "MTi1.h"
#include "simpleDataLink.h"
#include "messages.h"
#include "actuator_driver.h"
#include "pid_conversions.h"
#include "queue.h"
#include "tim.h"
#include <stm32l452xx.h>
#include <malloc.h>
#include "queue_structs.h"
#include "constants.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
Temp_values ntc_values;
PID_Inputs_struct PID_Inputs;
Actuator_struct Reaction1;
Actuator_struct Reaction2;
Actuator_struct MagneTorquer1;
Actuator_struct MagneTorquer2;
Actuator_struct MagneTorquer3;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t Channels_mask[NUM_DRIVERS] = {1,1,1,1,1};
uint8_t error_status = 0;


osThreadId FirstCheckTaskHandle;
uint32_t FirstCheckTaskBuffer[ stack_size ];//4096
osStaticThreadDef_t FirstCheckTaskControlBlock;

osThreadId ControlAlgorithmTaskHandle;
uint32_t ControlAlgorithmTaskBuffer[ stack_size ]; //4096
osStaticThreadDef_t ControlAlgorithmTaskControlBlock;

osThreadId IMUTaskHandle;
uint32_t IMUTaskBuffer[ stack_size]; //4096
osStaticThreadDef_t IMUTaskControlBlock;

osThreadId OBC_CommTaskHandle;
uint32_t OBC_CommTaskBuffer[ stack_size1 ]; //16384
osStaticThreadDef_t OBC_CommTaskControlBlock;

xSemaphoreHandle ADCSHouseKeepingMutex;
StaticSemaphore_t xADCSHouseKeepingMutexBuffer;
xSemaphoreHandle ControlMutex;
StaticSemaphore_t xControlMutexBuffer;
xSemaphoreHandle IMURead_ControlMutex;
StaticSemaphore_t xIMURead_ControlMutexBuffer;

osMessageQId ADCSHouseKeepingQueueHandle;
uint8_t ADCSHouseKeepingQueueBuffer[ 256 * sizeof( float ) ];
osStaticMessageQDef_t ADCSHouseKeepingQueueControlBlock;
osMessageQId IMUQueue1Handle;
uint8_t IMUQueue1Buffer[ 256 * sizeof( imu_queue_struct ) ];
osStaticMessageQDef_t IMUQueue1ControlBlock;
osMessageQId IMUQueue2Handle;
uint8_t IMUQueue2Buffer[ 256 * sizeof( imu_queue_struct ) ];
osStaticMessageQDef_t IMUQueue2ControlBlock;
osMessageQId setAttitudeADCSQueueHandle;
uint8_t setAttitudeADCSQueueBuffer[ 256 * sizeof( float ) ];
osStaticMessageQDef_t setAttitudeADCSQueueControlBlock;
osMessageQId setOpModeADCSQueueHandle;
uint8_t setOpModeADCSQueueBuffer[ 32 * sizeof( uint8_t ) ];
osStaticMessageQDef_t setOpModeADCSQueueControlBlock;


/* USER CODE END Variables */
osThreadId defaultTaskHandle;
uint32_t defaultTaskBuffer[ 128 ];
osStaticThreadDef_t defaultTaskControlBlock;

osSemaphoreId setAttitudeSemHandle;
osStaticSemaphoreDef_t setAttitudeSemControlBlock;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Check_current_temp(void const * argument);
void Control_Algorithm_Task(void const * argument);
void IMU_Task(void const * argument);
void OBC_Comm_Task(void const * argument);


//defining serial line I/O functions
//using UART driver
uint8_t txFunc1(uint8_t byte){
	return (sendDriver_UART(&huart1, &byte, 1)!=0);
}
uint8_t rxFunc1(uint8_t* byte){
	return (receiveDriver_UART(&huart1, byte, 1)!=0);
}

uint8_t txFunc4(uint8_t byte){
	return (sendDriver_UART(&huart4, &byte, 1)!=0);
}
uint8_t rxFunc4(uint8_t* byte){
	return (receiveDriver_UART(&huart4, byte, 1)!=0);
}

//defining tick function for timeouts
uint32_t sdlTimeTick(){
	return HAL_GetTick();
}


//defining putch to enable printf
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

/*PUTCHAR_PROTOTYPE{
   HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
   return ch;
}*/
PUTCHAR_PROTOTYPE{
	uint8_t c=(uint8_t)ch;
	sendDriver_UART(&huart2,&c,1);
	return c;
}


/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

	initDriver_UART();
	//UART1 = for printf
	addDriver_UART(&huart2,USART2_IRQn,keep_old);
	//UART1 = for OBC communication
	addDriver_UART(&huart1,USART1_IRQn,keep_old);
	//UART1 = for IMU
	addDriver_UART(&huart4,UART4_IRQn,keep_new);
	//UART3 = for Sun Sensors Acquisition
	//addDriver_UART(&huart3,USART3_IRQn,keep_old);

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
	ControlMutex = xSemaphoreCreateMutexStatic(&xControlMutexBuffer);
	configASSERT(ControlMutex);
	xSemaphoreGive(ControlMutex);
	IMURead_ControlMutex = xSemaphoreCreateMutexStatic(&xIMURead_ControlMutexBuffer);
	configASSERT(IMURead_ControlMutex);
	xSemaphoreGive(IMURead_ControlMutex);

  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of setAttitudeSem */
	osSemaphoreStaticDef(setAttitudeSem, &setAttitudeSemControlBlock);
	setAttitudeSemHandle = osSemaphoreCreate(osSemaphore(setAttitudeSem), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* definition and creation of IMUQueue1 */
	osMessageQStaticDef(IMUQueue1, 512, uint32_t,IMUQueue1Buffer, &IMUQueue1ControlBlock);
	IMUQueue1Handle = osMessageCreate(osMessageQ(IMUQueue1), NULL);
  /* definition and creation of IMUQueue2 */
	osMessageQStaticDef(IMUQueue2, 512, uint32_t, IMUQueue2Buffer, &IMUQueue2ControlBlock);
	IMUQueue2Handle = osMessageCreate(osMessageQ(IMUQueue2), NULL);
  /* definition and creation of ADCSHouseKeepingQueue */
	osMessageQStaticDef(ADCSHouseKeepingQueue, 512, uint32_t, ADCSHouseKeepingQueueBuffer, &ADCSHouseKeepingQueueControlBlock);
	ADCSHouseKeepingQueueHandle = osMessageCreate(osMessageQ(ADCSHouseKeepingQueue), NULL);
  /* definition and creation of setAttitudeADCSQueue */
	osMessageQStaticDef(setAttitudeADCSQueue, 512, uint32_t,setAttitudeADCSQueueBuffer, &setAttitudeADCSQueueControlBlock);
	setAttitudeADCSQueueHandle = osMessageCreate(osMessageQ(setAttitudeADCSQueue), NULL);
  /* definition and creation of setOpModeADCSQueue */
	osMessageQStaticDef(setOpModeADCSQueue, 32, uint32_t,setOpModeADCSQueueBuffer, &setOpModeADCSQueueControlBlock);
	setOpModeADCSQueueHandle = osMessageCreate(osMessageQ(setOpModeADCSQueue), NULL);

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
	osThreadStaticDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128, defaultTaskBuffer, &defaultTaskControlBlock);
	defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* definition and creation of FirstCheckTask */
    osThreadStaticDef(FirstCheckTask, Check_current_temp, osPriorityAboveNormal, 0, stack_size, FirstCheckTaskBuffer, &FirstCheckTaskControlBlock);
  	FirstCheckTaskHandle = osThreadCreate(osThread(FirstCheckTask), NULL);
  /* definition and creation of ControlAlgorithmTask */
	osThreadStaticDef(ControlAlgorithmTask, Control_Algorithm_Task, osPriorityNormal, 0,stack_size, ControlAlgorithmTaskBuffer, &ControlAlgorithmTaskControlBlock);
	ControlAlgorithmTaskHandle = osThreadCreate(osThread(ControlAlgorithmTask), NULL);
  /* definition and creation of IMUTask */
	osThreadStaticDef(IMUTask, IMU_Task, osPriorityNormal, 0,stack_size, IMUTaskBuffer, &IMUTaskControlBlock);
	IMUTaskHandle = osThreadCreate(osThread(IMUTask), NULL);
  /* definition and creation of OBC_CommTaskHandle */
	osThreadStaticDef(OBC_CommTask, OBC_Comm_Task, osPriorityAboveNormal, 0,stack_size1, OBC_CommTaskBuffer, &OBC_CommTaskControlBlock);
	OBC_CommTaskHandle = osThreadCreate(osThread(OBC_CommTask), NULL);


  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


void Check_current_temp(void const * argument)
{
  /* USER CODE BEGIN Check_pwr_temp */
	//declaring serial line
	//static serial_line_handle line;
	//Inizialize Serial Line for UART3
	//sdlInitLine(&line,&txFunc3,&rxFunc3,50,2);
	init_tempsens_handler(&ntc_values);
	volatile float currentbuf[NUM_ACTUATORS],voltagebuf[NUM_ACTUATORS];
	Current_Temp_Struct *local_current_temp_struct = (Current_Temp_Struct*) malloc(sizeof(Current_Temp_Struct));
	static uint8_t count = 0;
	
	/*Start calibration */
	if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) !=  HAL_OK)
	{
#if enable_printf
	  	printf("Error with ADC: not calibrated correctly \n");
#endif
	}

	/* Infinite loop */
	for(;;)
	{
		//volatile float prev = HAL_GetTick();
		//printf("We are in CHECK TASK \n");
#if enable_printf
		//printf("We are in CHECK TASK \n");
#endif
		//----------------------------------------------------------------------

		//GET TEMPERATURES------------------------------------------------------
		//float prev1 = HAL_GetTick();
		get_temperatures(&hspi2,&ntc_values,count);
		//float next1 = HAL_GetTick();
		//printf("Execussion of get_temperatures: %.1f ms\n",next1-prev1);
		count ++;
		//----------------------------------------------------------------------

		//GET ACTUATORS CURRENT
		get_actuator_current(&hadc1,voltagebuf,currentbuf,Channels_mask);
		/*for(int i=0;i<NUM_DRIVERS;i++)
		{
			printf("Actuator %d current value: %f",i,currentbuf[i]);
		}*/
		//----------------------------------------------------------------------

		//CHECK IF THEY ARE OK
		/*for(int i=0;i<NUM_ACTUATORS;i++)
		{
			if(i>=0 && i<2)
			{
				//Check Reaction Wheels currents
				if(current_buf[i] > 1.0f) //> 1A
				{
					printf("MAgnetorquer %d current value: %f is above threshold!!!! ",i,currentbuf[i]);
					error_status = 5;
				}
			}
			else{
				//Check MagneTorquers currents
				if(current_buf[i] > 0.05f) // >50mA
				{
					printf("Magnetorquer %d current value: %f is above threshold!!!! ",i,currentbuf[i]);
					error_status = 4;
				}
			}
		}

		for(int i=0;i<NUM_TEMP_SENS;i++)
		{
			if(ntc_values.temp[i]>50) //>50 gradi
			{
				printf("Temp %d value: %f is above threshold!!!! ",i,ntc_values.temp[i]);
				error_status = 3;
			}

		}
		 */
		switch(error_status)
		{
			case 0:
				//ALL IS OK
				//Send Housekeeping to OBC task
				
				if (local_current_temp_struct == NULL) {
#if enable_printf
					   printf("IMU TASK: allocazione struttura fallita !\n");
#endif
				}
				else
				{
					if(count == 8)
					{
						for(int i=0;i<NUM_ACTUATORS;i++)
						{
							local_current_temp_struct->current[i] = currentbuf[i];
#if enable_printf
							printf("Task check: Current n%d,value: %f,current vect:%f \n",i+1,local_current_temp_struct->current[i],currentbuf[i]);
#endif
			    		}
						for(int i=NUM_ACTUATORS;i<NUM_TEMP_SENS+NUM_ACTUATORS;i++)
						{
							local_current_temp_struct->temperature[i - NUM_ACTUATORS] = ntc_values.temp[i - NUM_ACTUATORS];
#if enable_printf
							printf("Task check: Temperature n%d,ntc value: %f,value: %f \n",i-4,ntc_values.temp[i-NUM_ACTUATORS],local_current_temp_struct->temperature[i-NUM_ACTUATORS]);
#endif
						}

						//Invio queue a OBC Task
						if (osMessagePut(ADCSHouseKeepingQueueHandle,(uint32_t)local_current_temp_struct,300) != osOK) {
#if enable_printf
			    		   	printf("Invio a OBC Task fallito \n");
#endif
			       			free(local_current_temp_struct); // Ensure the receiving task has time to process
						} else {
#if enable_printf
			    		    printf("Dati Inviati a OBC Task\n");
#endif
						}
						count = 0;
					}
				}
				break;
			case 1:
				break;
			case 2:
				break;
			case 3:
				//PROBLEM WITH TEMPERATURE SENSORS
				//Fare partire un interrupt
				break;
			case 4:
				//PROBLEM WITH MAGNETORQUERS
				//Fare partire un interrupt
				break;
			case 5:
				//PROBLEM WITH REACTION WHEELS
				//Fare partire un interrupt
				break;

		}
		//volatile next = HAL_GetTick();
		//printf("Execussion of check task: %.1f ms\n",next-prev);
	    osDelay(300);
	  }
  /* USER CODE END Check_pwr_temp */
}


void OBC_Comm_Task(void const * argument)
{
  /* USER CODE BEGIN OBC_Comm_Task */
	static serial_line_handle line1;
	//Inizialize Serial Line for UART1
	sdlInitLine(&line1,&txFunc1,&rxFunc1,50,2);

	uint8_t opmode=0;
	uint32_t rxLen;

	setAttitudeADCS *RxAttitude = (setAttitudeADCS*) malloc(sizeof(setAttitudeADCS));
	housekeepingADCS TxHousekeeping;
	attitudeADCS TxAttitude;
	setOpmodeADCS RxOpMode;
	//opmodeADCS TxOpMode;
	osEvent retvalue1,retvalue;
	uint8_t cnt1 = 0,cnt2 = 0;
	char rxBuff[SDL_MAX_PAY_LEN];

  /* Infinite loop */
  for(;;)
  {
	  //printf("We are in OBC TASK \n");
#if enable_printf
	  //printf("We are in OBC TASK \n");
#endif
	  /*-------------------RECEIVE FROM OBC-------------------------*/
	  //trying to receive a message
	  rxLen=sdlReceive(&line1,(uint8_t *)rxBuff,sizeof(rxBuff));
	  if(rxLen){
#if enable_printf
	  	printf("OBC TASK: Received %lu bytes !!!!!!!!!!!!\n",rxLen);
#endif
	  	if(rxBuff[0]==SETOPMODEADCS_CODE && rxLen==sizeof(setOpmodeADCS)){
#if enable_printf
	  		printf("Received setOpmodeADCS message\n");
#endif
	  		//setOpmodeADCS msgStruct;
	  		memcpy(&RxOpMode,rxBuff,sizeof(setOpmodeADCS));
	  		opmode=RxOpMode.opmode;
#if enable_printf
	  		printf("Opmode changed to %u\n",opmode);
#endif
	  	}else if(rxBuff[0]==SETATTITUDEADCS_CODE && rxLen==sizeof(setAttitudeADCS)){
#if enable_printf
  			printf("OBC TASK:Received setAttitudeADCS message!!!!!!!!!\n");
#endif
  			//do something...
  			//Send opMode = setattitudeadcs to Control Task
			if (RxAttitude == NULL) {
#if enable_printf
					   printf("OBC TASK: allocazione struttura RxAttitude fallita !\n");
#endif
			}
			else
			{
				memcpy(RxAttitude,rxBuff,sizeof(setAttitudeADCS));

				//Send Attitude Queue to Control Task
			 	if (osMessagePut(setAttitudeADCSQueueHandle,(uint32_t)RxAttitude,200) != osOK) {
#if enable_printf
			    	printf("Invio a Control Task fallito \n");
#endif
			       	free(RxAttitude); // Ensure the receiving task has time to process
				} else {
#if enable_printf
			        printf("Dati Inviati a Control Task\n");
#endif
			 	}
			}
	  		
	  	}else if(rxBuff[0]==ATTITUDEADCS_CODE && rxLen==sizeof(attitudeADCS)){
#if enable_printf
  			printf("Received attitudeADCS message\n");
#endif
  			//do something...
  			//(in theory this should never arrive to ADCS)
	  	}else if(rxBuff[0]==HOUSEKEEPINGADCS_CODE && rxLen==sizeof(housekeepingADCS)){
#if enable_printf
  			printf("Received housekeepingADCS message\n");
#endif
  			//do something...
  			//(in theory this should never arrive to ADCS)
	  	}else if(rxBuff[0]==OPMODEADCS_CODE && rxLen==sizeof(opmodeADCS)){
#if enable_printf
  			printf("Received opmodeADCS message\n");
#endif
  			//do something...
  			//(in theory this should never arrive to ADCS)
	  	}else{
#if enable_printf
  			printf("Message not recognized %u %lu\n",rxBuff[0],rxLen);
#endif
  		}
  	}
	  else
	  {
		  //printf("Didn't receive any packet \n");
#if enable_printf
		  printf("OBC TASK:  Message not recognized %u %lu\n",rxBuff[0],rxLen);
#endif
	  }

	 /*-------------------SEND TO OBC-------------------------*/
	//sampling
	  /* in theory here we should sample values and fill telemetry structures
	  telemetryStruct.temp1=...;
	  telemetryStruct.speed=...;
	  .....*/
	
	 //Receive HouseKeeping sensor values via Queue
	retvalue = osMessageGet(ADCSHouseKeepingQueueHandle,300);

	//printf("OBC Task: Tick_Time: %lu \n",HAL_GetTick());

	if (retvalue.status == osEventMessage)
	{
		cnt1++;
		processCombinedData((void*)&retvalue,(void *)&TxHousekeeping,receive_Current_Tempqueue_OBC);
		//attitude sampling
		//in this case we just send the local copy of the structure
		//ALWAYS remember to set message code (use the generated defines

		//printf("OBC: Trying to send attitude \n");
		//finally we send the message
		if(cnt1 == 1)
		{
			printf("OBC TASK: after 5 counts: %lu \n",HAL_GetTick());
			TxAttitude.code=ATTITUDEADCS_CODE;
			TxAttitude.ticktime=HAL_GetTick();
		if(sdlSend(&line1,(uint8_t *)&TxAttitude,sizeof(attitudeADCS),1)){
#if enable_printf
			printf("OBC : sent attitudeADCS bytes:%d \n",sizeof(attitudeADCS));
#endif
			/*sentAttitudeMessages++;
			if(sentAttitudeMessages==5)
			{
				printf("OBC: Sent %lu attitudeADCS messages bytes:%d\n",sentAttitudeMessages,sizeof(attitudeADCS));
				sentAttitudeMessages=0;
			}*/
			for(uint32_t y=0;y<sizeof(attitudeADCS);y++){
					//printf("%02x",((uint8_t *)&localAttitude)[y]);
			}
		}
		else{
#if enable_printf
			printf("OBC: Failed to send attitudeADCS \n");
#endif

		}
		cnt1 = 0;
		}
	}

	//Receive Telemetry IMU via Queue
	retvalue1 = osMessageGet(IMUQueue2Handle, 300);

	if (retvalue1.status == osEventMessage)
	{
		cnt2++;
		processCombinedData((void*)&retvalue1,(void *)&TxAttitude,receive_IMUqueue_OBC);
		//in this case we just fill the structure with random values
		//ALWAYS remember to set message code (use the generated defines
		if(cnt2 == 3)
		{
			printf("OBC TASK: after 7 counts: %lu \n",HAL_GetTick());
			TxHousekeeping.code=HOUSEKEEPINGADCS_CODE;
			TxHousekeeping.ticktime=HAL_GetTick();
			//printf("OBC: Trying to send housekeeping \n");
			//finally we send the message
		if(sdlSend(&line1,(uint8_t *)&TxHousekeeping,sizeof(housekeepingADCS),1))
		{
#if enable_printf
		  	printf("OBC: Sent housekeepingADCS bytes:%d \n",sizeof(housekeepingADCS));
#endif
		 	for(uint32_t y=0;y<sizeof(housekeepingADCS);y++){
		// 		printf("%02x",((uint8_t *)&TxHousekeeping)[y]);
			  }
		}
		else{
#if enable_printf
			printf("OBC: Failed to send housekeepingADCS \n");
#endif
		}
		cnt2 = 0;
		}

	}

	opmodeADCS opmodeMsg;
	opmodeMsg.opmode=opmode;
	//ALWAYS remember to set message code (use the generated defines
	opmodeMsg.code=OPMODEADCS_CODE;
	//finally we send the message (WITH ACK REQUESTED)
	//printf("OBC: Trying to send opmodeADCS \n");
	if(sdlSend(&line1,(uint8_t *)&opmodeMsg,sizeof(opmodeADCS),1))
	{
#if enable_printf
	  	printf("OBC : Sent opmodeADCS bytes:%d \n",sizeof(opmodeADCS));
#endif
	  for(uint32_t y=0;y<sizeof(opmodeADCS);y++){
	  	//printf("%02x",((uint8_t *)&opmodeMsg)[y]);
	  }
	//  printf("\n");
	}
	else{
#if enable_printf
		printf("OBC: Failed to send opmodeADCS \n");
#endif
	}


  	osDelay(400);
  }
  /* USER CODE END OBC_Comm_Task */
}

void Control_Algorithm_Task(void const * argument)
{
  /* USER CODE BEGIN Control_Algorithm_Task */
	uint8_t flag = 0;
	osEvent retvalue,retvalue1;

	//Inizialize actuators struct
	init_actuator_handler(&Reaction1,&htim1,TIM_CHANNEL_1,TIM_CHANNEL_2,100000,50); //100 khz
	init_actuator_handler(&Reaction2,&htim2,TIM_CHANNEL_3,TIM_CHANNEL_4,20000,50);
	init_actuator_handler(&MagneTorquer1,&htim3,TIM_CHANNEL_1,TIM_CHANNEL_2,89000,50); //89 khz
	init_actuator_handler(&MagneTorquer2,&htim3,TIM_CHANNEL_3,TIM_CHANNEL_4,10000,50);
	init_actuator_handler(&MagneTorquer3,&htim2,TIM_CHANNEL_1,TIM_CHANNEL_2,94000,50); //94 khz

	//Inizialize PID struct
	PID_INIT(&PID_Inputs);


	/* Infinite loop */
	for(;;)
	{
		//printf("We are in Control Algorithm TASK \n");
#if enable_printf
		//printf("We are in Control Algorithm TASK \n");
#endif
		//Receive Telemetry IMU via Queue

		retvalue1 = osMessageGet(setAttitudeADCSQueueHandle,200);
		processCombinedData((void*)&retvalue1,(void *)&PID_Inputs,receive_Attitudequeue_control);

		retvalue = osMessageGet(IMUQueue1Handle, 300);
		processCombinedData((void*)&retvalue,(void *)&PID_Inputs,receive_IMUqueue_control);

		
		//ALGORITHM
		//PID_main(&PID_Inputs);

		//Update PWM values
		//X Magnetorquer

		if(!flag)
		{
//			actuator_START(&Reaction1);
//			actuator_START(&Reaction2);
//			actuator_START(&MagneTorquer1);
//			actuator_START(&MagneTorquer2);
//			actuator_START(&MagneTorquer3);
			flag = 1;
		}


		//No change dir:
		//update_duty_dir(&Reaction1,50,0);
		//Change dir :
		//update_duty_dir(&MagneTorquer1,70,1);



		//X Magnetorquer
		//Change dir :
		//update_duty_dir(&MagneTorquer1,PID_Inputs.th_Dutycycle[0],1);
		//No change dir:
		//update_duty_dir(&MagneTorquer1,PID_Inputs.th_Dutycycle[0],0);
		//Y Magnetorquer
		//Change dir :
		//update_duty_dir(&MagneTorquer1,PID_Inputs.th_Dutycycle[1],1);
		//No change dir:
		//update_duty_dir(&MagneTorquer1,PID_Inputs.th_Dutycycle[1],0);
		//Z Magnetorquer
		//Change dir :
		//update_duty_dir(&MagneTorquer1,PID_Inputs.th_Dutycycle[2],1);
		//No change dir:
		//update_duty_dir(&MagneTorquer1,PID_Inputs.th_Dutycycle[2],0);



		/*if (xSemaphoreTake(IMURead_ControlMutex, (TickType_t)10) == pdTRUE) //If control don't read IMU
		{
			printf("Control Task : Taken IMURead_ControlMutex control");
			//Spegnere i magnetorquer
			xSemaphoreGive(IMURead_ControlMutex);
			printf("Control Task : Released IMURead_ControlMutex control");
		}
		*/
		osDelay(500);
	}
  /* USER CODE END Control_Algorithm_Task */
}

void IMU_Task(void const * argument)
{
  /* USER CODE BEGIN IMU_Task */
#if enable_printf
	printf("Initializing IMU \n");
#endif
	//uint8_t ret = 1;
	uint8_t ret = initIMUConfig(&huart4);
#if enable_printf
	if(ret) printf("IMU correctly configured \n");
	else printf("Error configuring IMU \n");
#endif

	float gyro[3]={1,2,3};
	float mag[3]={4,5,6};
	float acc[3] = {7,8,9};

	imu_queue_struct *local_imu_struct =(imu_queue_struct*) malloc(sizeof(imu_queue_struct));

	/* Infinite loop */
	for(;;)
	{
		//printf("We are in IMU TASK \n");
#if enable_printf
		//printf("We are in IMU TASK \n");
#endif

		//Non c'è bisogno di settare o resettare il CTS e l'RTS della UART4 per IMU perchè le funzioni
		//UART_Transmit e UART_Receive gestiscono la cosa automaticamente se dall'altro lato il dispositivo ha abilitato pure
		//queste due linee per l'UART
		//Se voglio far comunicare IMU e Nucleo con solo le 2 linee UART tx ed Rx basta che disabilito l'hardware flow control
		//da CubeMx.


		ret=readIMUPacket(&huart4, gyro, mag, acc, 500); //mag measured in Gauss(G) unit -> 1G = 10^-4 Tesla
		mag[0]/=10000; //1G = 10^-4 Tesla
		mag[1]/=10000; //1G = 10^-4 Tesla
		mag[2]/=10000; //1G = 10^-4 Tesla
		/*if (xSemaphoreTake(IMURead_ControlMutex, (TickType_t)10) == pdTRUE)//If reading IMU DO NOT CONTROL
		{
			printf("IMU Task : Taken IMURead_Control control");
			ret=readIMUPacket(&huart4, gyro, mag, 50);
			xSemaphoreGive(IMURead_ControlMutex);
			printf("IMU Task : Released IMURead_Control control");
		}*/
		if(ret)
		{
#if enable_printf
			printf("IMU correctly sampled:\n ");
			printf("IMU: Giro x: %f,Giro y: %f,Giro z: %f \n",gyro[0],gyro[1],gyro[2]);
			printf("IMU: Magn x: %f,Magn y: %f,Magn z: %f \n",mag[0],mag[1],mag[2]);
#endif
			/*for(uint32_t field=0; field<3;field++){
					printf("%f \t",gyro[field]);
			}
			printf("\nMagnetometer: ");
			for(uint32_t field=0; field<3;field++){
				printf("%f \t",mag[field]);
			}
			printf("\n");*/

			if (local_imu_struct == NULL) {
#if enable_printf
					   printf("IMU TASK: allocazione struttura fallita !\n");
#endif
			}
			else
			{
				//Riempio struct con valori letti da IMU,per poi inviareli a Task Controllo
				for (int i = 0; i < 3; i++)
				{
					local_imu_struct->gyro_msr[i] = gyro[i];
#if enable_printf
					//printf("IMU TASK: Giro[%d] : %f \n",i,local_imu_struct->gyro_msr[i]);
#endif
					local_imu_struct->mag_msr[i] = mag[i];
					local_imu_struct->acc_msr[i] = acc[i];
					printf("AAAAAAAAAAAAAAAAAAAAAAA  Accelerometer axis %d, value %f AAAAAAAAAAAAAA", i, acc[i]);
#if enable_printf
					//printf("IMU TASK: Magn Field[%d] : %f \n",i,local_imu_struct->mag_msr[i]);
#endif
				}
			
				//Invio queue a Control Task
			 	if (osMessagePut(IMUQueue1Handle,(uint32_t)local_imu_struct,300) != osOK) {
#if enable_printf
			    	printf("Invio a Control Task fallito \n");
#endif
			       	free(local_imu_struct); // Ensure the receiving task has time to process
				} else {
#if enable_printf
			        printf("Dati Inviati a Control Task \n");
#endif
			 	}
			 	//Invio queue a OBC Task
			 	if (osMessagePut(IMUQueue2Handle,(uint32_t)local_imu_struct,300) != osOK) {
#if enable_printf
			    	printf("Invio a OBC Task fallito \n");
#endif
			       	free(local_imu_struct); // Ensure the receiving task has time to process
			 	} else {
#if enable_printf
			    	printf("Dati a Control Inviati \n");
#endif
				}
			}
		}
		else
#if enable_printf
			printf("IMU: Error configuring IMU \n");
#endif
		osDelay(1000);
	}
  /* USER CODE END IMU_Task */
}
/* USER CODE END Application */
