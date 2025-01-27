// G8RTOS_Threads.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for thread functions.

/************************************Includes***************************************/

#include "./threads.h"

#include "./MultimodDrivers/multimod.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/************************************Includes***************************************/

/*************************************Defines***************************************/

#define BUFFER_SIZE 12

// ADJUST THIS AS NEEDED. THIS IS BASED ON IF THE CAMERA LOOKS LIKE IT IS POINTING DIRECTLY AT ME
#define TARGET_X 170
#define TARGET_Y 150
/*************************************Defines***************************************/

/*********************************Global Variables**********************************/

// Global data structure for keeping track of data
char buffer[BUFFER_SIZE];
uint8_t currData = 0;
uint8_t autonomous = 0;


/*********************************Global Variables**********************************/

/*************************************Threads***************************************/

void Idle_Thread(void) {
    while(1);
}

void FaceDetect(void) {
    SysCtlDelay(1);

    // Declare variables
    // Previous values for quick drawing
    int x = 10, y = 10, h = 10, w = 10;

    while(1) {
        // Wait for data. This is done 4 times because the UART Int needs to trigger 4 times for x, y, h, and w.
        G8RTOS_WaitSemaphore(&sem_UART);

        // Draw over old values to save on computation time.
        G8RTOS_WaitSemaphore(&sem_SPIA);
        ST7789_DrawHLine(x,y,w,ST7789_BLACK);
        ST7789_DrawHLine(x,y+h,w,ST7789_BLACK);
        ST7789_DrawVLine(x,y,h, ST7789_BLACK);
        ST7789_DrawVLine(x+w,y,h, ST7789_BLACK);
        G8RTOS_SignalSemaphore(&sem_SPIA);
        
        // Read in data
        y = G8RTOS_ReadFIFO(UART_FIFO) * 4 / 5;
        x = G8RTOS_ReadFIFO(UART_FIFO) * 4 / 5;

        w = G8RTOS_ReadFIFO(UART_FIFO);
        h = G8RTOS_ReadFIFO(UART_FIFO);
        if(!(x == -2 || y == -2 || w == -2 || h == -2)){
            UARTprintf("X value: %d\nY value: %d\nW value: %d\nH value: %d\n", x, y, w, h);

            // draw a rectangle on LCD using the new data from above
            G8RTOS_WaitSemaphore(&sem_SPIA);
            ST7789_DrawHLine(x,y,w,ST7789_BLUE);
            ST7789_DrawHLine(x,y+h,w,ST7789_BLUE);
            ST7789_DrawVLine(x,y,h, ST7789_BLUE);
            ST7789_DrawVLine(x+w,y,h, ST7789_BLUE);
            G8RTOS_SignalSemaphore(&sem_SPIA);
        }

        
        sleep(10);
    }
}


void Read_Buttons(void) {
    // Initialize / declare any variables here
    uint8_t buttons;

    while(1) {

        // wait for button semaphore
        G8RTOS_WaitSemaphore(&sem_PCA9555_Debounce);
        // debounce buttons
        sleep(10);
        // Get buttons
        G8RTOS_WaitSemaphore(&sem_I2CA);
        buttons = MultimodButtons_Get();
        G8RTOS_SignalSemaphore(&sem_I2CA);
        
        //flip the logic if the button was pressed
        if(buttons & SW1){
            G8RTOS_WaitSemaphore(&sem_autonomous);
            autonomous = ~autonomous & 0x1;
            G8RTOS_SignalSemaphore(&sem_autonomous);
        }
        
        // clear button interrupt
        GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
        GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    }
}
// with out PWM signal, it is running around 50 hz. This means our resolution is SERIOUSLY decreased. We have around 20 available optios for motor control.
// Lower Bound: 192 = ~900 us
// Upper Bound: 422 = ~2 ms

void Motor_Control(void){
    //previous PWM signal for bottom motor(X) 12 bits of resolution
    float x = 0;
    uint16_t x_pwm_val = 307;      //corresponds to ~90 degrees
    uint16_t prev_x_pwm_val = 0;   // saves time if there has been no additional input

    // previous PWM signal for top motor (Y) 12-bit resolution
    float y = 0;
    uint16_t y_pwm_val = 307;      //corresponds to ~90 degrees
    uint16_t prev_y_pwm_val = 0;      //previous value to make sure were only writing when we need to.
    
    // PID parameters. We need Kp, Ki and Kd for the system, and then the errors for each x and y for pan/tilt
    float Kpy = 0.05;
    float Kpx = 0.06;// Ki = 0.005, Kd = 0.001;
    float x_pos, y_pos, width, height;
    float x_error = 0, y_error = 0;
//    float x_integral = 0, y_integral = 0;
//    float x_derivative = 0, y_derivative = 0;
//    float x_prev_error = 0, y_prev_error = 0;


    int16_t resultX;
    int16_t resultY;

    // I want to make sure the motors are in roughly the 90 degree position, so they can swing almost equally in either direction
    PCA9685_WritePWM0(x_pwm_val);
    PCA9685_WritePWM1(y_pwm_val);
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);

    // This is used to avoid simultaneous access to the autonomous var
    uint8_t autoVar = 0;

    while(1){
        // in order to avoid simultaneous access to autonomous, use a semaphore and a local variable to check if button was pressed
        G8RTOS_WaitSemaphore(&sem_autonomous);
        if(autonomous){
            autoVar = 1;
        }else{
            autoVar = 0;
        }
        G8RTOS_SignalSemaphore(&sem_autonomous);


        // IF OPTIMIZATIONS ARE NEEDED, THIS SECTION CAN BE IMPROVED
        // read value from FIFOs to keep them clear
        resultY = G8RTOS_ReadFIFO(JOYSTICKY_FIFO);
        resultX = G8RTOS_ReadFIFO(JOYSTICKX_FIFO);
        // check to see if they have no values, if they skip this iteration
        if(resultX == -2 | resultY == -2){
            resultX = 2100;
            resultY = 2100;
        }


        if(autoVar){
            x_pos = G8RTOS_ReadFIFO(FACE_FIFO);
            y_pos = G8RTOS_ReadFIFO(FACE_FIFO);
            // since there will be a height and weight added to fifo too, just do 2 arbitrary reads to get rid of the data
            width = G8RTOS_ReadFIFO(FACE_FIFO);
            height = G8RTOS_ReadFIFO(FACE_FIFO);
            if (x_pos != -2 && y_pos != -2 && height != -2 && width != -2) {
                x_error = TARGET_X - (x_pos + width/2);
                y_error = TARGET_Y - (y_pos + height/2);

//                // PID calculations
//                x_integral += x_error;
//                y_integral += y_error;
//
//                x_derivative = x_error - x_prev_error;
//                y_derivative = y_error - y_prev_error;
                if(abs(x_error) > 8){
                    float x_pid = Kpx * x_error; // + Ki * x_integral + Kd * x_derivative;
                    x_pwm_val = constrain(x_pwm_val - x_pid, 192.0, 420.0); // Constrain to servo range
                    PCA9685_WritePWM0(x_pwm_val);
                }
                if(abs(y_error) > 8){
                    float y_pid = Kpy * y_error; // + Ki * y_integral + Kd * y_derivative;
                    y_pwm_val = constrain(y_pwm_val + y_pid, 192.0, 420.0);
                    PCA9685_WritePWM1(y_pwm_val);
                }



//                x_prev_error = x_error;
//                y_prev_error = y_error;
            }
        }else{
            // If joystick axis within deadzone, set to 0. Otherwise normalize it.
            if(resultX < 2150 & resultX > 2000){
                x = 0;
            }else{
                x = (resultX-2048)/4096.0;
            }

            if(resultY > 2000 & resultY < 2150){
                y = 0;
            }else{
                y = (resultY-2048)/4096.0;
            }


            // change the degrees, but need to calculate the duty cycle to send
            //x is minus because of its orientation on the board
            x_pwm_val = x_pwm_val + 20*x;
            y_pwm_val = y_pwm_val - 20*y;
            if(x_pwm_val > 422){
                x_pwm_val = 422;
            }else if(x_pwm_val < 192){
                x_pwm_val = 192;
            }
            if(y_pwm_val > 422){
                y_pwm_val = 422;
            }else if(y_pwm_val < 192){
                y_pwm_val = 192;
            }

            // only write changes if there are difference values
            if(prev_x_pwm_val != x_pwm_val){
                PCA9685_WritePWM0(x_pwm_val);
            }
            if(prev_y_pwm_val != y_pwm_val){
                PCA9685_WritePWM1(y_pwm_val);
            }
            prev_x_pwm_val = x_pwm_val;
            prev_y_pwm_val = y_pwm_val;
        }
        // sleep
        sleep(2);
    }
}


/********************************Periodic Threads***********************************/

// Moving joystick down increases the PWM0 pulse width.
// moving joystick left increases the PWM1 pulse width.
// increasing PWM pulse width moves motor clockwise
void Get_Joystick(void) {
    // Read the joystick
    G8RTOS_WriteFIFO(JOYSTICKX_FIFO, JOYSTICK_GetX());
    G8RTOS_WriteFIFO(JOYSTICKY_FIFO, JOYSTICK_GetY());
    // Send through FIFO.
}

/********************************Periodic Threads***********************************/


/*******************************Aperiodic Threads***********************************/

void UART4_Handler() {

    // Prepare to read data
    UARTIntDisable(UART4_BASE, UART_INT_RX);

    // Get interrupt status
    uint8_t count = 0;
    char temp;
    uint16_t output;
    while(UARTCharsAvail(UART4_BASE)){
    // Continue reading if there is still data
        // Store current data value
        if(count % 2 == 0){
            temp = (UARTCharGet(UART4_BASE));
        }else{
            output = (UARTCharGet(UART4_BASE)) << 8 | temp;
            G8RTOS_WriteFIFO(UART_FIFO, output);
            if(autonomous){
                G8RTOS_WriteFIFO(FACE_FIFO, output);
            }
        }
        count++;

    }
    // Signal data ready only after everything has been recieved
    G8RTOS_SignalSemaphore(&sem_UART);
    // Clear the asserted interrupts
    UARTIntClear(UART4_BASE, UART_INT_RX);
    UARTIntEnable(UART4_BASE, UART_INT_RX);
    
}

void Button_Handler() {

    // disable interrupt and signal semaphore
    GPIOIntDisable(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);
}

/*******************************Aperiodic Threads***********************************/


float constrain(float value, float min, float max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}
