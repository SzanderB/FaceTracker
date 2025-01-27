// threads.h
// Date Created: 2023-07-26
// Date Updated: 2023-07-26
// Threads

#ifndef THREADS_H_
#define THREADS_H_

/************************************Includes***************************************/

#include "./G8RTOS/G8RTOS.h"
/************************************Includes***************************************/

/*************************************Defines***************************************/
#define UART_FIFO       0
#define JOYSTICKX_FIFO  1
#define JOYSTICKY_FIFO  2
#define FACE_FIFO       3

semaphore_t sem_SPIA;
semaphore_t sem_UART;
semaphore_t sem_PCA9555_Debounce;
semaphore_t sem_I2CA;
semaphore_t sem_autonomous;
/*************************************Defines***************************************/

/***********************************Semaphores**************************************/



/***********************************Semaphores**************************************/

/***********************************Structures**************************************/


/***********************************Structures**************************************/


/*******************************Background Threads**********************************/

void Idle_Thread(void);

void FaceDetect(void);

void Motor_Control(void);

void Read_Buttons(void);
/*******************************Background Threads**********************************/

/********************************Periodic Threads***********************************/
void Get_Joystick(void);
/********************************Periodic Threads***********************************/

/*******************************Aperiodic Threads***********************************/

void UART4_Handler(void);
void Button_Handler();

/*******************************Aperiodic Threads***********************************/
float constrain(float value, float min, float max);

#endif /* THREADS_H_ */

