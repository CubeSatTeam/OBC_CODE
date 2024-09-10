//This is the c file for implementation of functions regarding Sensors handling
#include "sensors.h"

//Variables
uint8_t single_mode_pckt[2] = {0x10,0x86};
//uint8_t WRITE_ON_MR = 0x10; //The first byte to send on DIN to CR to start Conversation in Single MODE
//uint8_t MR_FOR_SINGLE_MOD = 0x86; //The second byte to send on DIN to start Conversation in Single MODE
uint8_t READ_DATAREG = 0x38; //The byte to send on DIN to start obtain the result of Conversation on Dout
uint8_t READ_STATUSREG = 0x08; //The byte to send on DIN to the CR to obtain the content of Status Reg

void init_tempsens_handler(Temp_values *Temp_values){
    Temp_values->temp[0] = 0;
    Temp_values->temp[1] = 0;
    Temp_values->temp[2] = 0;
    Temp_values->temp[3] = 0;
    Temp_values->temp[4] = 0;
    Temp_values->temp[5] = 0;
    Temp_values->temp[6] = 0;
    Temp_values->temp[7] = 0;
    Temp_values->values.R[0] = 10040; //ohm
    Temp_values->values.R[1] = 10020; //ohm
    Temp_values->values.R[2] = 10000; //ohm
    Temp_values->values.R[3] = 10020; //ohm
    Temp_values->values.R[4] = 10000; //ohm
    Temp_values->values.R[5] = 10010; //ohm
    Temp_values->values.R[6] = 10000; //ohm
    Temp_values->values.R[7] = 10000; //ohm
    Temp_values->values.R_25 = 10000; //ohm
    Temp_values->values.B = 3977; //k
    Temp_values->values.Vdd = 3.3; //v
}

void select_input(uint8_t sel)
{
    //s3 must be put to 0 always,otherwise the mux would not put out the signal
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); //s3 = 0
    switch (sel)
    {
    case 0:
        /* code */
        //Select Y0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); //s2 = 0
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); //s1 = 0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); //s0 = 0
        
        
        break;
    case 1:
        /* code */
        //Select Y1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); //s2 = 0
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); //s1 = 0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); //s0 = 1
        break;
    case 2:
        /* code */
        //Select Y2
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); //s2 = 0
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); //s1 = 1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); //s0 = 0
        break;
    case 3:
        /* code */
        //Select Y3
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); //s2 = 0
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); //s1 = 1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); //s0 = 1
        break;
    case 4:
        /* code */
        //Select Y4
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); //s2 = 1
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); //s1 = 0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); //s0 = 0
        break;
    case 5:
        /* code */
        //Select Y5
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); //s2 = 1
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); //s1 = 0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); //s0 = 1
        break;
    case 6:
        /* code */
        //Select Y6
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); //s2 = 1
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); //s1 = 1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); //s0 = 0
        break;
    case 7:
        /* code */
        //Select Y7
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); //s2 = 1
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); //s1 = 1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); //s0 = 1
        break;

    default:
        /* code */
        //Error, print that the sel value is not correct
    	printf("Error: selection signal is NOT CORRECT!!! \n");
        break;
    }
}

float ADC_Conversion(SPI_HandleTypeDef *spi_struct,uint8_t mode)
{
    uint8_t spi_data[2];
    volatile uint16_t dec_data;
    //uint8_t status_reg_val;
    uint32_t time_start = 0; //ms
    uint32_t timeout = 100; //ms
    volatile float data = 0;
    if(mode == 0) //Continuous conversion mode
    {
#if enable_printf
    	printf("Continuous conversion mode \n");
#endif
    	//CS LOW: Enable communication
    	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
    	//time_start = HAL_GetTick() + 100;
    	//while(HAL_GetTick()<time_start);
    	HAL_SPI_Transmit(spi_struct,&READ_DATAREG, 1,timeout);
    	HAL_SPI_Receive(spi_struct,spi_data, 2,timeout);
    	//CS HIGH: Disable communication
    	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
    	dec_data = (spi_data[0]<<8)|spi_data[1];
    	data = ((float)dec_data/pow(2,N))*Vref;
#if enable_printf
        printf("Transmitted packet and received bytes: %d, data =  %f v \n",dec_data,data);
#endif
        return data;
    }
    else if(mode == 1) //Single conversion mode
    {
#if enable_printf
    	printf("Single conversion mode \n");
#endif
    	//CS LOW: Enable communication
    	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);

    	HAL_SPI_Transmit(spi_struct,single_mode_pckt, 2, timeout);
    	time_start = HAL_GetTick() + 200;
    	//printf("time_start : %lu, cpu time: %lu \n",time_start,HAL_GetTick());
    	while(HAL_GetTick()<time_start);
    	//Get the result of conversion
    	HAL_SPI_Transmit(spi_struct,&READ_DATAREG, 1,timeout);
    	HAL_SPI_Receive(spi_struct,spi_data, 2,timeout);
    	//HAL_SPI_TransmitReceive(spi_struct,&READ_DATAREG,spi_data,1,timeout+100);
    	//CS HIGH: Disable communication
    	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
    	//Process result of conversuion
   		dec_data = (spi_data[0]<<8)|spi_data[1];
   		data = (((float)dec_data)/(pow(2,N)-1))*Vref;
#if enable_printf
   		printf("Transmitted packet and received bytes: %d, data =  %f v \n",dec_data,data);
#endif
   		return data;
    }
    else if(mode == 2) //Continuous read Mode
    {
#if enable_printf
    	printf("Continuous read mode \n");
#endif
		return 1;
    }
    else{
        //Stampa che non è stato inserito il mode corretto perchè deve essere compreso tra 0 e 2
    	printf("Error: mode value must between 0 and 2!!! \n");

    	return 0;
    }

}

void voltage_to_temperature_conv(float value,Temp_values *s1,uint8_t i){

	//printf("Value: %f, B: %f, Vdd: %f, R_25: %f, R: %f \n",value,s1->values.B,s1->values.Vdd,s1->values.R_25,s1->values.R[i]);
	s1->temp[i] = ((298.15 * s1->values.B)/(s1->values.B - (298.15*(ln(((s1->values.Vdd/value)-1)*(s1->values.R_25/s1->values.R[i])))))) - 273.15;
	//printf("Temperature: %f \n",s1->temp[i]);
}

void get_temperatures(SPI_HandleTypeDef *spi_struct,Temp_values *temp_struct, uint8_t counter)
{
	volatile float conv_result;
	switch(counter)
	{
		case 0:

		select_input(0);
		//Single conversion mode
		conv_result = ADC_Conversion(spi_struct,1);
		voltage_to_temperature_conv(conv_result,temp_struct,0);
#if enable_printf
		printf("Sensors 1 Temp : %.2f \n",temp_struct->temp[0]);
#endif
		//HAL_Delay(100);

		break;

		case 1:

		select_input(1);
		//Single conversion mode
		conv_result = ADC_Conversion(spi_struct,1);
		voltage_to_temperature_conv(conv_result,temp_struct,1);
#if enable_printf
		printf("Sensors 2 Temp : %.2f \n",temp_struct->temp[1]);
#endif
		//HAL_Delay(100);

		break;

		case 2:

		select_input(2);
		//Single conversion mode
		conv_result = ADC_Conversion(spi_struct,1);
		voltage_to_temperature_conv(conv_result,temp_struct,2);
#if enable_printf
		printf("Sensors 3 Temp : %.2f \n",temp_struct->temp[2]);
#endif
		//HAL_Delay(100);

		break;

		case 3:

		select_input(3);
		//Single conversion mode
		conv_result = ADC_Conversion(spi_struct,1);
		voltage_to_temperature_conv(conv_result,temp_struct,3);
#if enable_printf
		printf("Sensors 4 Temp : %.2f \n",temp_struct->temp[3]);
#endif
		//HAL_Delay(100);
		break;

		case 4:

		select_input(4);
		//Single conversion mode
		conv_result = ADC_Conversion(spi_struct,1);
		voltage_to_temperature_conv(conv_result,temp_struct,4);
#if enable_printf
		printf("Sensors 5 Temp : %.2f \n",temp_struct->temp[4]);
#endif
		//HAL_Delay(100);

		break;
		case 5:

		select_input(5);
		//Single conversion mode
		conv_result = ADC_Conversion(spi_struct,1);
		voltage_to_temperature_conv(conv_result,temp_struct,5);
#if enable_printf
		printf("Sensors 6 Temp : %.2f \n",temp_struct->temp[5]);
#endif
		//HAL_Delay(100);

		break;

		case 6:

		select_input(6);
		//Single conversion mode
		conv_result = ADC_Conversion(spi_struct,1);
		voltage_to_temperature_conv(conv_result,temp_struct,6);
#if enable_printf
		printf("Sensors 7 Temp : %.2f \n",temp_struct->temp[6]);
#endif
		//HAL_Delay(100);

		break;

		case 7:

		select_input(7);
		//Single conversion mode
		conv_result = ADC_Conversion(spi_struct,1);
		voltage_to_temperature_conv(conv_result,temp_struct,7);
#if enable_printf
		printf("Sensors 8 Temp : %.2f \n",temp_struct->temp[7]);
#endif
		//HAL_Delay(100);

		break;

		default:
#if enable_printf
		printf("CHECK TASK: get_temperatures -> Nothing is happening");
#endif
		break;

	}

}
