// Lab 6, uP2 Fall 2023
// Created: 2023-07-31
// Updated: 2023-08-01
// Lab 6 is intended to serve as an introduction to the BeagleBone Black and wireless
// communication concepts.

/************************************Includes***************************************/

#include "G8RTOS/G8RTOS.h"
#include "./MultimodDrivers/multimod.h"

#include "./threads.h"

/************************************MAIN*******************************************/

int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    multimod_init();
    G8RTOS_Init();

    // Initialize Semaphores
    G8RTOS_InitSemaphore(&sem_UART, 1);
    G8RTOS_InitSemaphore(&sem_SPIA, 1);
    G8RTOS_InitSemaphore(&sem_PCA9555_Debounce, 1);
    G8RTOS_InitSemaphore(&sem_I2CA, 1);
    G8RTOS_InitSemaphore(&sem_autonomous, 1);


    G8RTOS_InitFIFO(UART_FIFO);
    G8RTOS_InitFIFO(JOYSTICKX_FIFO);
    G8RTOS_InitFIFO(JOYSTICKY_FIFO);
    G8RTOS_InitFIFO(FACE_FIFO);
    
    // Add background threads
    G8RTOS_AddThread(Idle_Thread, 254, "Idle\n");
    G8RTOS_AddThread(FaceDetect, 254, "Faces");
    G8RTOS_AddThread(Read_Buttons, 253, "Buttons\0");
    G8RTOS_AddThread(Motor_Control, 252, "Motors\0");


    // Add periodic threads
    G8RTOS_Add_PeriodicEvent(Get_Joystick, 50, 9);
    
    // Add aperiodic threads
    G8RTOS_Add_APeriodicEvent(UART4_Handler, 4, 76);
    G8RTOS_Add_APeriodicEvent(Button_Handler, 5, 20);
    
    G8RTOS_Launch();
    while(1);
}

/************************************MAIN*******************************************/
