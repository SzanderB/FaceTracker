// multimod_PCA9555b.h
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// GPIO Expander Header File

#ifndef MULTIMOD_PWM_H_
#define MULTIMOD_PWM_H_

/************************************Includes***************************************/

#include <stdint.h>
#include <stdbool.h>

#include <inc/hw_memmap.h>
#include <inc/hw_gpio.h>

#include <driverlib/i2c.h>
#include <driverlib/gpio.h>

/************************************Includes***************************************/

/*************************************Defines***************************************/


#define PCA9685_ADDR          0x55       //0b1010101 then R/(!W)
#define PCA9685_PRESCALER_ADDR      0xFE
#define PCA9685_50HZ_PRESCALER      0x79

#define PCA9685_LED0_PWM_ON_L       0x06
#define PCA9685_LED0_PWM_ON_H       0x07
#define PCA9685_LED0_PWM_OFF_L      0x08
#define PCA9685_LED0_PWM_OFF_H      0x09
#define PCA9685_LED1_PWM_ON_L       0x0A
#define PCA9685_LED1_PWM_ON_H       0x0B
#define PCA9685_LED1_PWM_OFF_L      0x0C
#define PCA9685_LED1_PWM_OFF_H      0x0D

#define PCA9685_MODE1_ADDR          0x00
#define PCA9685_AUTO_INC            0x20
#define PCA9685_SLEEP               0x10
#define PCA9685_UNSLEEP             0x00



/*************************************Defines***************************************/

/******************************Data Type Definitions********************************/
/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/
/****************************Data Structure Definitions*****************************/

/***********************************Externs*****************************************/
/***********************************Externs*****************************************/

/********************************Public Variables***********************************/
/********************************Public Variables***********************************/

/********************************Public Functions***********************************/

void PCA9685_Init(void);
void PCA9685_WritePWM0(uint16_t pwm_val);
void PCA9685_WritePWM1(uint16_t pwm_val);


/********************************Public Functions***********************************/

/*******************************Private Variables***********************************/
/*******************************Private Variables***********************************/

/*******************************Private Functions***********************************/
/*******************************Private Functions***********************************/

#endif /* MULTIMOD_PWM */


