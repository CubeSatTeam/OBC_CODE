#ifndef MESSAGES_H
#define MESSAGES_H

/*
 Automatically generated by parseMessages.py
 from messages.json
*/

#include <stdint.h>

// message name: opmodeADCS code: 20
#define OPMODEADCS_CODE 20
typedef struct {
	uint8_t code;
	uint8_t opmode;
}__attribute__((packed)) opmodeADCS;

// message name: attitudeADCS code: 21
#define ATTITUDEADCS_CODE 21
typedef struct {
	uint8_t code;
	float omega_x;
	float omega_y;
	float omega_z;
	float b_x;
	float b_y;
	float b_z;
	float theta_x;
	float theta_y;
	float theta_z;
	float suntheta_x;
	float suntheta_y;
	float suntheta_z;
	uint32_t ticktime;
}__attribute__((packed)) attitudeADCS;

// message name: housekeepingADCS code: 22
#define HOUSEKEEPINGADCS_CODE 22
typedef struct {
	uint8_t code;
	float temperature[8];
	uint16_t temperatureRAW[8];
	float current[5];
	uint16_t currentRAW[5];
	uint32_t ticktime;
}__attribute__((packed)) housekeepingADCS;

// message name: setOpmodeADCS code: 0
#define SETOPMODEADCS_CODE 0
typedef struct {
	uint8_t code;
	uint8_t opmode;
}__attribute__((packed)) setOpmodeADCS;

// message name: setAttitudeADCS code: 1
#define SETATTITUDEADCS_CODE 1
typedef struct {
	uint8_t code;
	float deltaomega_x; //desired
	float deltaomega_y; //desired
	float deltaomega_z; //desired
	float deltab_x;
	float deltab_y;
	float deltab_z;
	float deltatheta_x;
	float deltatheta_y;
	float deltatheta_z;
}__attribute__((packed)) setAttitudeADCS;

#endif
