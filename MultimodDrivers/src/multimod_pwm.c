// multimod_OPT3001.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for OPT3001 functions

/************************************Includes***************************************/

#include "../multimod_pwm.h"

#include <stdint.h>
#include <driverlib/sysctl.h>
#include "../multimod_i2c.h"

/************************************Includes***************************************/

/********************************Public Functions***********************************/
void PCA9685_Init(void){
    I2C_Init(I2C_A_BASE);

    uint8_t data_buffer[2] = {PCA9685_MODE1_ADDR, PCA9685_SLEEP};
    // set auto increment feature enabled in MODE 1 register for ease of writing
    I2C_WriteMultiple(I2C_A_BASE, PCA9685_ADDR, data_buffer, 2);
    // set PWM clock divide to provide around a 2.1ms clock rate
    data_buffer[0] = PCA9685_PRESCALER_ADDR;
    data_buffer[1] = PCA9685_50HZ_PRESCALER;
    I2C_WriteMultiple(I2C_A_BASE, PCA9685_ADDR, data_buffer, 2);

    //wake it back up
    data_buffer[0] = PCA9685_MODE1_ADDR;
    data_buffer[1] = PCA9685_AUTO_INC;
    I2C_WriteMultiple(I2C_A_BASE, PCA9685_ADDR, data_buffer, 2);

//    uint8_t data_read;
//    I2C_WriteSingle(I2C_A_BASE, PCA9685_ADDR, PCA9685_PRESCALER_ADDR);
//    data_read = I2C_ReadSingle(I2C_A_BASE, PCA9685_ADDR);

    // Enable clock to port E
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    // Wait for it to turn out
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE))
    {
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_5);

}


// takes in PWM value, makes sure its within the 4095 range.
// Because my servo takes 900 us as the lower bound, and our PWM is around 2100 us period, the minimum value is 1755
// Calculates the time on/time off values to write and then writes them
// Input is duty cycle out of 4096, which corresponds to 0-180 degrees
// The pulse width is what determines what the degree is, from 900 us to 2100 us
// the actual values 0-4095 will be split up into the 900-2100 us range for proper use
void PCA9685_WritePWM0(uint16_t pwm_val){

    //check bounds
//    if(pwm_val > 4095){
//        pwm_val = 4095;
//    }else if(pwm_val < 1755){
//        pwm_val = 1755;
//    }

    // start is always going to be 0, and the turn off value will be changed accordingly
    // get the time off, which is still a 12-bit value
    uint8_t packet[3] = {PCA9685_LED0_PWM_OFF_L, pwm_val & 0xFF, ((pwm_val >> 8) & 0xFF)};
    I2C_WriteMultiple(I2C_A_BASE, PCA9685_ADDR, packet, 3);
}

void PCA9685_WritePWM1(uint16_t pwm_val){

    //check bounds
    // if(pwm_val > 4095){
    //     pwm_val = 4095;
    // }else if(pwm_val < 1755){
    //     pwm_val = 1755;
    // }

    // start is always going to be 0, and the turn off value will be changed accordingly
    // get the time off, which is still a 12-bit value
    uint8_t packet[3] = {PCA9685_LED1_PWM_OFF_L, pwm_val & 0xFF, ((pwm_val >> 8) & 0xFF)};
    I2C_WriteMultiple(I2C_A_BASE, PCA9685_ADDR, packet, 3);
}

/********************************Public Functions***********************************/
