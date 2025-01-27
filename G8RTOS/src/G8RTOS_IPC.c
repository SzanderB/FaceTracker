// G8RTOS_IPC.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for FIFO functions for interprocess communication

#include "../G8RTOS_IPC.h"

/************************************Includes***************************************/

#include "../G8RTOS_Semaphores.h"

/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/

typedef struct G8RTOS_FIFO_t {
    uint32_t buffer[FIFO_SIZE];
    uint32_t *head;
    uint32_t *tail;
    uint32_t lostData;
    semaphore_t currentSize;
    semaphore_t mutex;
} G8RTOS_FIFO_t;


/***********************************Externs*****************************************/

/********************************Private Variables***********************************/

static G8RTOS_FIFO_t FIFOs[MAX_NUMBER_OF_FIFOS];


/********************************Public Functions***********************************/

// G8RTOS_InitFIFO
// Initializes FIFO, points head & tai to relevant buffer
// memory addresses. Returns - 1 if FIFO full, 0 if no error
// Param uint32_t "FIFO_index": Index of FIFO block
// Return: int32_t
int32_t G8RTOS_InitFIFO(uint32_t FIFO_index) {
    // Check if FIFO index is out of bounds
    if(FIFO_index >= MAX_NUMBER_OF_FIFOS){
        return -1;
    }
    // Init head, tail pointers
    // start of buffer is at buffer
    FIFOs[FIFO_index].head = FIFOs[FIFO_index].buffer;
    FIFOs[FIFO_index].tail = FIFOs[FIFO_index].buffer;
    // Init the mutex, current size
    G8RTOS_InitSemaphore(&(FIFOs[FIFO_index].currentSize), 0);
    G8RTOS_InitSemaphore(&(FIFOs[FIFO_index].mutex), 1);
    // Init lost data
    FIFOs[FIFO_index].lostData = 0;

    return 0;
}

// G8RTOS_ReadFIFO
// Reads data from head pointer of FIFO.
// Param uint32_t "FIFO_index": Index of FIFO block
// Return: int32_t
int32_t G8RTOS_ReadFIFO(uint32_t FIFO_index) {
    G8RTOS_WaitSemaphore(&(FIFOs[FIFO_index].mutex));
    //check to see if there's data
    if(FIFOs[FIFO_index].currentSize == 0){
        G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].mutex));
        return -2;
    }
    // check if index is valid
    if(FIFO_index >= MAX_NUMBER_OF_FIFOS){
        G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].mutex));
        return -1;
    }
    int32_t data = *(FIFOs[FIFO_index].head);
    FIFOs[FIFO_index].head +=1;
    // if its at the end, loop
    if(FIFOs[FIFO_index].head == &(FIFOs[FIFO_index].buffer[FIFO_SIZE])){
        FIFOs[FIFO_index].head = &(FIFOs[FIFO_index].buffer[0]);
    }
    G8RTOS_WaitSemaphore(&(FIFOs[FIFO_index].currentSize));
    G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].mutex));

    return data;
}

// G8RTOS_WriteFIFO
// Writes data to tail of buffer.
// 0 if no error, -1 if out of bounds, -2 if full
// Param uint32_t "FIFO_index": Index of FIFO block
// Param uint32_t "data": data to be written
// Return: int32_t
int32_t G8RTOS_WriteFIFO(uint32_t FIFO_index, uint32_t data) {
    //signal wait for this specific FIFO
    G8RTOS_WaitSemaphore(&(FIFOs[FIFO_index].mutex));
    // check if there is space
    if(FIFOs[FIFO_index].currentSize > FIFO_SIZE-1){
        FIFOs[FIFO_index].lostData +=1;
        G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].mutex));
        return -2;
    }
    // check if if the index is valid
    if(FIFO_index >= MAX_NUMBER_OF_FIFOS){
        FIFOs[FIFO_index].lostData +=1;
        G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].mutex));
        return -1;
    }
    //Place the data
    *(FIFOs[FIFO_index].tail) = data;
    FIFOs[FIFO_index].tail += 1;
    // if its at the last value of the fifo, wrap around
    if(FIFOs[FIFO_index].tail == &(FIFOs[FIFO_index].buffer[FIFO_SIZE])){
        FIFOs[FIFO_index].tail = &(FIFOs[FIFO_index].buffer[0]);
    }
    G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].currentSize));
    G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].mutex));


    return 0;
 }

