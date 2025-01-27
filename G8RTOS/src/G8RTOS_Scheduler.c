// G8RTOS_Scheduler.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for scheduler functions

#include "../G8RTOS_Scheduler.h"

/************************************Includes***************************************/
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../G8RTOS_CriticalSection.h"

#include <inc/hw_memmap.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

// #define EC

/********************************Private Variables**********************************/

// Thread Control Blocks - array to hold information for each thread
static tcb_t threadControlBlocks[MAX_THREADS];

// Thread Stacks - array of arrays for individual stacks of each thread
static uint32_t threadStacks[MAX_THREADS][STACKSIZE];

// Periodic Event Threads - array to hold pertinent information for each thread
static ptcb_t pthreadControlBlocks[MAX_PTHREADS];

// Current Number of Threads currently in the scheduler
static uint32_t NumberOfThreads;

// Current Number of Periodic Threads currently in the scheduler
static uint32_t NumberOfPThreads;

// this threadCounter will tally the total number of threads in the array, alive or dead
static uint32_t threadCounter = 0;
// Number of dead threads, for use in keeping track of the threadCounter
static uint32_t deadCounter = 0;


/*******************************Private Functions***********************************/

// Occurs every 1 ms.
static void InitSysTick(void)
{
    // hint: use SysCtlClockGet() to get the clock speed without having to hardcode it!
    // Set systick period to overflow every 1 ms.
    SysTickPeriodSet(SysCtlClockGet()/1000);
    // Set systick interrupt handler
    SysTickIntRegister(SysTick_Handler);
    // Set pendsv handler
    IntRegister(FAULT_PENDSV, PendSV_Handler);
    // Enable systick interrupt
    SysTickIntEnable();
    // Enable systick
    SysTickEnable();
}


/********************************Public Variables***********************************/

uint32_t SystemTime;

tcb_t* CurrentlyRunningThread;



/********************************Public Functions***********************************/

// SysTick_Handler
// Increments system time, sets PendSV flag to start scheduler.
// Return: void
void SysTick_Handler() {
    SystemTime++;

    // Traverse the linked-list to find which threads should be awake.

    tcb_t * pt = CurrentlyRunningThread;

    do{
        pt = pt->nextTCB;
        if(pt->asleep && (pt->sleepCount <= SystemTime)){
            pt->asleep = false;
        }
    }while(CurrentlyRunningThread != pt);


    for(int i= 0; i < NumberOfPThreads; i++){
        if(pthreadControlBlocks[i].executeTime == SystemTime || pthreadControlBlocks[i].currentTime >= pthreadControlBlocks[i].period){
            pthreadControlBlocks[i].handler();
            pthreadControlBlocks[i].currentTime = 0;
            break;
        }
        pthreadControlBlocks[i].currentTime++;
    }

    // Traverse the periodic linked list to run which functions need to be run.
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}

// G8RTOS_Init
// Initializes the RTOS by initializing system time.
// Return: void
void G8RTOS_Init() {
    uint32_t newVTORTable = 0x20000000;
    uint32_t* newTable = (uint32_t*)newVTORTable;
    uint32_t* oldTable = (uint32_t*) 0;

    for (int i = 0; i < 155; i++) {
        newTable[i] = oldTable[i];
    }

    HWREG(NVIC_VTABLE) = newVTORTable;

    SystemTime = 0;
    NumberOfThreads = 0;
    NumberOfPThreads = 0;
}

// G8RTOS_Launch
// Launches the RTOS.
// Return: error codes, 0 if none
int32_t G8RTOS_Launch() {
    // Initialize system tick
    InitSysTick();
    // Set currently running thread to the first control block
    CurrentlyRunningThread = &threadControlBlocks[0];
    // Set interrupt priorities
       // Pendsv
        IntPrioritySet(FAULT_PENDSV, 0xE0);
       // Systick
        IntPrioritySet(FAULT_SYSTICK, 0xE0);
    // Call G8RTOS_Start()
    G8RTOS_Start();
    return 0;
}

// G8RTOS_Scheduler
// Chooses next thread in the TCB. This time uses priority scheduling.
// Return: void
void G8RTOS_Scheduler() {
    // Using priority, determine the most eligible thread to run that
    // is not blocked or asleep. Set current thread to this thread's TCB.
    tcb_t* selected = CurrentlyRunningThread;
    // CurrentlyRunningThread = CurrentlyRunningThread->nextTCB;
    tcb_t *pt = CurrentlyRunningThread->nextTCB;
    uint16_t maxPrio = UINT8_MAX + 1;

    for(int i = 0; i < NumberOfThreads-1; i++){
        if(pt->asleep || (pt->blocked != NULL)){
            pt = pt->nextTCB;
            continue;
        }else if (pt->priority < maxPrio)
        {
            selected = pt;
            maxPrio = pt->priority;
        }
        pt = pt->nextTCB;
    }
    CurrentlyRunningThread = selected;
}

// G8RTOS_AddThread
// Adds a thread. This is now in a critical section to support dynamic threads.
// It also now should initalize priority and account for live or dead threads.
// Param void* "threadToAdd": pointer to thread function address
// Param uint8_t "priority": priority from 0, 255.
// Param char* "name": character array containing the thread name.
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name) {
    IBit_State = StartCriticalSection();
    // If number of threads is greater than the maximum number of threads
    if(NumberOfThreads >= MAX_THREADS){
        // return
        EndCriticalSection(IBit_State);
        return THREAD_LIMIT_REACHED;
    } else{
    // else
        // if no threads
        // This should only trigger once, since there will never be allowed to have 0 active threads again
        if(NumberOfThreads == 0){
            // Next and Previous TCBs will be itself
            threadControlBlocks[0].previousTCB = &threadControlBlocks[0];
            threadControlBlocks[0].nextTCB = &threadControlBlocks[0];
            // Init the stack
            SetInitialStack(0);
            // Set the PC to the function desired
            threadStacks[0][STACKSIZE-2] = (uint32_t)(threadToAdd);

            // add name and priority
            strncpy(threadControlBlocks[0].threadName, name, sizeof(threadControlBlocks[0].threadName) - 1);
            threadControlBlocks[0].threadName[sizeof(threadControlBlocks[0].threadName) - 1] = '\0'; // Ensure null-termination
            threadControlBlocks[0].priority = priority;
            threadControlBlocks[0].isAlive = true;

            threadControlBlocks[0].ThreadID = 0;
            
            // Increment #threads
            NumberOfThreads += 1;
            threadCounter++;
        }else{
            for(int i = 0; i < NumberOfThreads; i++){
                // check to see if there are any dead threads in the middle of the pack, if so replace it
                if(threadControlBlocks[i].isAlive == false){
                    //update the name, priority, and PC associated with it
                    threadStacks[i][STACKSIZE-2] = (uint32_t)(threadToAdd);
                    threadControlBlocks[i].priority = priority;

                    strncpy(threadControlBlocks[NumberOfThreads].threadName, name, sizeof(threadControlBlocks[NumberOfThreads].threadName) - 1);
                    threadControlBlocks[NumberOfThreads].threadName[sizeof(threadControlBlocks[NumberOfThreads].threadName) - 1] = '\0'; // Ensure null-termination
                    
                    //update the next and previous threads of the current thread AND update the next thread of the previous one, and the previous thread of the next one
                        // need to loop through to find the first one that isnt dead
                    for(int j = i+1; j < threadCounter; j++){
                        if(threadControlBlocks[j].isAlive){
                            threadControlBlocks[i].nextTCB = &threadControlBlocks[j];
                            threadControlBlocks[j].previousTCB = &threadControlBlocks[i];
                            break;
                        }
                    }

                    //change the previous TCB
                    for(int j = i-1; j >= 0; j--){
                        if(threadControlBlocks[j].isAlive){
                            threadControlBlocks[i].previousTCB = &threadControlBlocks[j];
                            threadControlBlocks[j].nextTCB = &threadControlBlocks[i];
                            break;
                        }
                    }
                    //BUT if there are no alive threads after it, it needs to loop until it finds the next one, starting at the front of the list
                    // this should cover all possibilities, like if you have 6 dead threads, and 2 live ones (after this function), and you place the new live thread after the only other live thread
                    // Should only trigger if the first thread is still alive, and all the rest are dead
                    if(threadControlBlocks[i].previousTCB == NULL){
                        for(int j = threadCounter-1; j > i; j--){
                            if(threadControlBlocks[j].isAlive){
                                threadControlBlocks[i].previousTCB = &threadControlBlocks[j];
                                threadControlBlocks[j].nextTCB = &threadControlBlocks[i];
                                break;
                            }
                        }
                    }
                    
                    // make sure it is marked alive again
                    threadControlBlocks[i].isAlive = true;
                    // decrement deadCounter
                    deadCounter--;
                    NumberOfThreads += 1;
                    EndCriticalSection(IBit_State);
                    return NO_ERROR;

                }
            }
            // else
                /*
                Append the new thread to the end of the linked list
                * 1. Number of threads will refer to the newest thread to be added since the current index would be NumberOfThreads-1
                * 2. Set the next thread for the new thread to be the first in the list, so that round-robin will be maintained
                * 3. Set the current thread's nextTCB to be the new thread
                * 4. Set the first thread's previous thread to be the new thread, so that it goes in the right spot in the list
                * 5. Point the previousTCB of the new thread to the current thread so that it moves in the correct order
                */
                // 2
                threadControlBlocks[NumberOfThreads].nextTCB = &threadControlBlocks[0];
                // 3
                threadControlBlocks[NumberOfThreads-1].nextTCB = &threadControlBlocks[NumberOfThreads];
                // 4
                threadControlBlocks[0].previousTCB = &threadControlBlocks[NumberOfThreads];
                // 5
                threadControlBlocks[NumberOfThreads].previousTCB = &threadControlBlocks[NumberOfThreads-1];

                SetInitialStack(NumberOfThreads);

                threadStacks[NumberOfThreads][STACKSIZE-2] = (uint32_t)(threadToAdd);
                
                // add name and priority
                // threadControlBlocks[NumberOfThreads].threadName = name;
                threadControlBlocks[NumberOfThreads].priority = priority;
                strncpy(threadControlBlocks[NumberOfThreads].threadName, name, sizeof(threadControlBlocks[NumberOfThreads].threadName) - 1);
                threadControlBlocks[NumberOfThreads].threadName[sizeof(threadControlBlocks[NumberOfThreads].threadName) - 1] = '\0'; // Ensure null-termination
                
                threadControlBlocks[NumberOfThreads].ThreadID = NumberOfThreads;
                
                // the one thing my check above does not cover is if the last thread in the list was the one that was killed
                // Just check if dead counter is 0, if it is, then it is a completely new thread and needs to be tracked
                if(deadCounter == 0){
                    threadCounter++;
                }else{
                    // there are no longer any dead threads
                    deadCounter -= 1;
                }
                threadControlBlocks[NumberOfThreads].isAlive = true;
                NumberOfThreads += 1;
        }
        EndCriticalSection(IBit_State);
        return NO_ERROR;
    }

}

// SetInitialStack
// Initializes each newly added thread's stack to arbitrary values
// Input: i, the number of thread in the list of threads
// Return: void
void SetInitialStack(int i){
    threadControlBlocks[i].stackPointer = &threadStacks[i][STACKSIZE-16]; // thread stack pointer
    threadStacks[i][STACKSIZE-1] = 0x01000000; // Thumb bit
    threadStacks[i][STACKSIZE-3] = 0x14141414; // R14
    threadStacks[i][STACKSIZE-4] = 0x12121212; // R12
    threadStacks[i][STACKSIZE-5] = 0x03030303; // R3
    threadStacks[i][STACKSIZE-6] = 0x02020202; // R2
    threadStacks[i][STACKSIZE-7] = 0x01010101; // R1
    threadStacks[i][STACKSIZE-8] = 0x00000000; // R0
    threadStacks[i][STACKSIZE-9] = 0x11111111; // R11
    threadStacks[i][STACKSIZE-10] = 0x10101010; // R10
    threadStacks[i][STACKSIZE-11] = 0x09090909; // R9
    threadStacks[i][STACKSIZE-12] = 0x08080808; // R8
    threadStacks[i][STACKSIZE-13] = 0x07070707; // R7
    threadStacks[i][STACKSIZE-14] = 0x06060606; // R6
    threadStacks[i][STACKSIZE-15] = 0x05050505; // R5
    threadStacks[i][STACKSIZE-16] = 0x04040404; // R4
}

// G8RTOS_Add_APeriodicEvent


// Param void* "AthreadToAdd": pointer to thread function address
// Param int32_t "IRQn": Interrupt request number that references the vector table. [0..155].
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_Add_APeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, int32_t IRQn) {
    // Disable interrupts
    IBit_State = StartCriticalSection();
    // Check if IRQn is valid
    if(!(IRQn <= 155 && IRQn >= 16)){
        return IRQn_INVALID;
    }
    // Check if priority is valid
    if(priority >= 7){
        return HWI_PRIORITY_INVALID;
    }
    // Set corresponding index in interrupt vector table to handler.
    IntRegister(IRQn, AthreadToAdd);
    // Set priority.
    IntPrioritySet(IRQn, priority);
    // Enable the interrupt.
    IntEnable(IRQn);
    // End the critical section.
    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

// G8RTOS_Add_PeriodicEvent
// Adds periodic threads to G8RTOS Scheduler
// Function will initialize a periodic event struct to represent event.
// The struct will be added to a linked list of periodic events
// Param void* "PThreadToAdd": void-void function for P thread handler
// Param uint32_t "period": period of P thread to add
// Param uint32_t "execution": When to execute the periodic thread
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_Add_PeriodicEvent(void (*PThreadToAdd)(void), uint32_t period, uint32_t execution) {
    // your code
    IBit_State = StartCriticalSection();
    // Make sure that the number of PThreads is not greater than max PThreads.
    if(NumberOfPThreads >= MAX_PTHREADS){
        EndCriticalSection(IBit_State);
        return THREAD_LIMIT_REACHED;
    }
    // Check if there is no PThread. Initialize and set the first PThread.
    if(NumberOfPThreads == 0){
        pthreadControlBlocks[0].handler = PThreadToAdd;
        pthreadControlBlocks[0].executeTime = execution;
        pthreadControlBlocks[0].period = period;
        pthreadControlBlocks[0].nextPTCB = &pthreadControlBlocks[0];
        pthreadControlBlocks[0].previousPTCB = &pthreadControlBlocks[0];
    }else{
        // Subsequent PThreads should be added, inserted similarly to a doubly-linked linked list
            // last PTCB should point to first, last PTCB should point to last.
        pthreadControlBlocks[NumberOfPThreads].nextPTCB = &pthreadControlBlocks[0];
        pthreadControlBlocks[NumberOfPThreads].previousPTCB = &pthreadControlBlocks[NumberOfPThreads-1];

        pthreadControlBlocks[0].previousPTCB = &pthreadControlBlocks[NumberOfPThreads];
        // Set function
        pthreadControlBlocks[NumberOfPThreads].handler = PThreadToAdd;
        // Set period
        pthreadControlBlocks[NumberOfPThreads].period = period;
        // Set execute time
        pthreadControlBlocks[NumberOfPThreads].executeTime = execution;
    }
        
    // Increment number of PThreads
    NumberOfPThreads++;

    EndCriticalSection(IBit_State);

    return NO_ERROR;
}

// G8RTOS_KillThread
// Param uint32_t "threadID": ID of thread to kill
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_KillThread(threadID_t threadID) {
    // Start critical section
    IBit_State = StartCriticalSection();
    // Check if there is only one thread, return if so
    if(NumberOfThreads == 1){
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }
    // Traverse linked list, find thread to kill
    tcb_t *pt = CurrentlyRunningThread;
    for(int i = 0; i < NumberOfThreads; i++){
        if(pt->ThreadID == threadID){

            // Update the next tcb and prev tcb pointers if found
            pt->previousTCB->nextTCB = pt->nextTCB;
            pt->nextTCB->previousTCB = pt->previousTCB;

            pt->previousTCB = NULL;
                // mark as not alive, release the semaphore it is blocked on
            pt->isAlive = false;
            if(pt->blocked){
                G8RTOS_SignalSemaphore((pt->blocked));
            }
            
            
            if(pt == CurrentlyRunningThread){
                HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
            }
            EndCriticalSection(IBit_State);
            return NO_ERROR;
        }
        pt = pt->nextTCB;
    }

    EndCriticalSection(IBit_State);
    return THREAD_DOES_NOT_EXIST;
}

// G8RTOS_KillSelf
// Kills currently running thread.
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_KillSelf() {
    // your code
    IBit_State = StartCriticalSection();
    // Check if there is only one thread, return if so
    if(NumberOfThreads == 1){
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }
    // Update the next tcb and prev tcb pointers if found
    CurrentlyRunningThread->previousTCB->nextTCB = CurrentlyRunningThread->nextTCB;
    CurrentlyRunningThread->nextTCB->previousTCB = CurrentlyRunningThread->previousTCB;

    CurrentlyRunningThread->previousTCB = NULL;
    // mark as not alive, release the semaphore it is blocked on
    CurrentlyRunningThread->isAlive = false;
    if(CurrentlyRunningThread->blocked){
        //G8RTOS_SignalSemaphore((CurrentlyRunningThread->blocked));
        *(CurrentlyRunningThread->blocked) = 0;
    }
    NumberOfThreads--;
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

// sleep
// Puts current thread to sleep
// Param uint32_t "durationMS": how many systicks to sleep for
void sleep(uint32_t durationMS) {
    // Update time to sleep to
    // Set thread as asleep
    CurrentlyRunningThread->sleepCount = durationMS + SystemTime;
    CurrentlyRunningThread->asleep = true;

    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}

// G8RTOS_GetThreadID
// Gets current thread ID.
// Return: threadID_t
threadID_t G8RTOS_GetThreadID(void) {
    return CurrentlyRunningThread->ThreadID;        //Returns the thread ID
}

// G8RTOS_GetNumberOfThreads
// Gets number of threads.
// Return: uint32_t
uint32_t G8RTOS_GetNumberOfThreads(void) {
    return NumberOfThreads;         //Returns the number of threads
}


void OS_Suspend(void){
    // either reset system time or dont
    // reseting messes with periodic threads, not resetting screws over the following thread
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}
