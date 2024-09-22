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
		for(int i=0;i<3;i++){

				int_pid_struct->angSpeed_Measured[i] = int_queue_struct->gyro_msr[i];
				int_pid_struct->B[i] = int_queue_struct->mag_msr[i];
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
		int_attitude_struct->omega_x = int_queue_struct->gyro_msr[0];
		int_attitude_struct->omega_y = int_queue_struct->gyro_msr[1];
		int_attitude_struct->omega_z = int_queue_struct->gyro_msr[2];
		int_attitude_struct->acc_x = int_queue_struct->acc_msr[0];
		int_attitude_struct->acc_y = int_queue_struct->acc_msr[1];
		int_attitude_struct->acc_z = int_queue_struct->acc_msr[2];
		int_attitude_struct->b_x = int_queue_struct->mag_msr[0];
		int_attitude_struct->b_y = int_queue_struct->mag_msr[1];
		int_attitude_struct->b_z = int_queue_struct->mag_msr[2];
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
		for(int i=0;i<NUM_ACTUATORS+NUM_TEMP_SENS;i++)
		{
			if(i<NUM_ACTUATORS){
	    			int_HK_struct->current[i] = int_queue_struct->current[i];
			}
	    	else
	    		if(i<NUM_ACTUATORS+NUM_TEMP_SENS){
	    			int_HK_struct->temperature[i - NUM_ACTUATORS] = int_queue_struct->temperature[i - NUM_ACTUATORS];

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
}
