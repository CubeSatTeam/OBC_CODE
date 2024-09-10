/*
 * queue_structs.c
 *
 *  Created on: 9 mag 2024
 *      Author: Utente
 */

#include "queue_structs.h"



void processCombinedData(void *event,void *strct1, CombinedDataProcessor processor) {
    processor(event,strct1);
}

void receive_IMUqueue_control(void *event,void *PID_struct) {

	imu_queue_struct *int_queue_struct;
	PID_Inputs_struct *int_pid_struct = (PID_Inputs_struct *)PID_struct;

	if (((osEvent *)event)->status == osEventMessage)
	{
		int_queue_struct = (imu_queue_struct *)((osEvent *) event)->value.p;
#if enable_printf
		printf("Control TASK: Received IMU measured values via Queue \n");
#endif
		for(int i=0;i<3;i++){

				int_pid_struct->angSpeed_Measured[i] = int_queue_struct->gyro_msr[i];
#if enable_printf
				printf("Control: Giro[%d] : %f \n",i,int_pid_struct->angSpeed_Measured[i]);
#endif
				int_pid_struct->B[i] = int_queue_struct->mag_msr[i];
#if enable_printf
				printf("Control: Magn Field[%d] : %f \n",i,int_pid_struct->B[i]);
#endif
		}
		free(int_queue_struct);
	}
	else
	{
		printf("Control Task: Ricezione IMU fallita con status: %d \n\n", ((osEvent *)event)->status);
	}
}

void receive_IMUqueue_OBC(void *event,void *attitude) {

	imu_queue_struct *int_queue_struct;
	attitudeADCS *int_attitude_struct = (attitudeADCS *)attitude;

	if (((osEvent *)event)->status == osEventMessage)
	{
		int_queue_struct = (imu_queue_struct *)((osEvent *) event)->value.p;
#if enable_printf
		printf("OBC TASK: Received IMU measured values via Queue \n");
#endif
		int_attitude_struct->omega_x = int_queue_struct->gyro_msr[0];
		int_attitude_struct->omega_y = int_queue_struct->gyro_msr[1];
		int_attitude_struct->omega_z = int_queue_struct->gyro_msr[2];
		int_attitude_struct->b_x = int_queue_struct->mag_msr[0];
		int_attitude_struct->b_y = int_queue_struct->mag_msr[1];
		int_attitude_struct->b_z = int_queue_struct->mag_msr[2];
#if enable_printf
		printf("OBC: Giro[0] : %f \n",int_attitude_struct->omega_x);
		printf("OBC: Giro[1] : %f \n",int_attitude_struct->omega_y);
		printf("OBC: Giro[2] : %f \n",int_attitude_struct->omega_z);
		printf("OBC: Magn Field[0] : %f \n",int_attitude_struct->b_x);
		printf("OBC: Magn Field[1] : %f \n",int_attitude_struct->b_y);
		printf("OBC: Magn Field[2] : %f \n",int_attitude_struct->b_z);
#endif
		free(int_queue_struct);
		}
		else
		{
			printf("OBC TASK:Ricezione IMU fallita con status: %d \n\n", ((osEvent *)event)->status);
		}
}
void receive_Current_Tempqueue_OBC(void *event,void *current_temp_struct)
{
	Current_Temp_Struct *int_queue_struct;
	housekeepingADCS *int_HK_struct = (housekeepingADCS *)current_temp_struct;
	if (((osEvent *)event)->status == osEventMessage)
	{
		int_queue_struct = (Current_Temp_Struct *)((osEvent *) event)->value.p;
#if enable_printf
		printf("OBC TASK: Received Currents and Temperatures values via Queue \n");
#endif
		for(int i=0;i<NUM_ACTUATORS+NUM_TEMP_SENS;i++)
		{
			if(i<NUM_ACTUATORS){
	    			int_HK_struct->current[i] = int_queue_struct->current[i];
#if enable_printf
	    			printf("OBC Task: Actuator %d current: %f Current_Temp_buff: %f \n",i+1,int_HK_struct->current[i],int_queue_struct->current[i]);
#endif
			}
	    	else
	    		if(i<NUM_ACTUATORS+NUM_TEMP_SENS){
	    			int_HK_struct->temperature[i - NUM_ACTUATORS] = int_queue_struct->temperature[i - NUM_ACTUATORS];
#if enable_printf
	    			printf("OBC Task: Temperature n%d value: %f Current_Temp_buff: %f \n",i - 4,int_HK_struct->temperature[i - NUM_ACTUATORS],int_queue_struct->temperature[i - NUM_ACTUATORS]);
#endif
	    		}
		}
		free(int_queue_struct);
	}
	else
	{
		printf("OBC TASK: Ricezione correnti e temperature fallita con status: %d \n\n", ((osEvent *)event)->status);
	}

}

void receive_Attitudequeue_control(void *event,void * PID_struct)
{
	setAttitudeADCS *int_attitude_adcs;
	PID_Inputs_struct *int_PID_struct = (PID_Inputs_struct *)PID_struct;
	if(((osEvent *)event)->status == osEventMessage)
	{	
#if enable_printf
		printf("Control TASK: Received AttitudeADCS values via Queue \n");
#endif
		int_attitude_adcs = (setAttitudeADCS *)((osEvent *) event)->value.p;
		int_PID_struct->angSpeed_Desired[0] = int_attitude_adcs->deltaomega_x; //desired
		int_PID_struct->angSpeed_Desired[1] = int_attitude_adcs->deltaomega_y; //desired
		int_PID_struct->angSpeed_Desired[2] = int_attitude_adcs->deltaomega_z; //desired
		for(int i=0;i<3;i++)
		{
#if enable_printf
			printf("Control Task: int_PID_struct->angSpeed_Desired[%d]: %f \n",i,int_PID_struct->angSpeed_Desired[i]);
#endif
		}
		//Trasformare campo magnetico desiderato in corrente desiderata
		//int_PID_struct->current_Desired[0]  = int_attitude_adcs->deltab_x; //desired
		//int_PID_struct->current_Desired[1]  = int_attitude_adcs->deltab_y; //desired
		//int_PID_struct->current_Desired[2]  = int_attitude_adcs->deltab_y; //desired
	}
	else{
		printf("Control Task: Ricezione AttitudeADCS fallita con status: %d \n\n", ((osEvent *)event)->status);
	}
}
