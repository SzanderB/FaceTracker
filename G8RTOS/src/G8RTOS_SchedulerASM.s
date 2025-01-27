; G8RTOS_SchedulerASM.s
; Created: 2022-07-26
; Updated: 2022-07-26
; Contains assembly functions for scheduler.

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc

	; Load the address of RunningPtr
	LDR R4, RunningPtr
	; Load the address of the thread control block of the currently running pointer
	LDR R5, [R4]
	; Load the first thread's stack pointer
	LDR R6, [R5]
	ADD r6, R6, #60
	STR R6, [R5]
	MOV SP, R6
	LDR LR, [R6, #-4]


	CPSIE I ; Enable interrupts at processor level

	BX LR				;Branches to the first thread	

	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:

	.asmfunc
		CPSID I
	PUSH {R4-R11}

	LDR R4, RunningPtr
	LDR R5, [R4]
	STR SP, [R5]

	PUSH {R0, LR}

	BL G8RTOS_Scheduler

	POP {R0, LR}

	LDR R5, [R4]
	LDR SP, [R5]

	POP {R4-R11}

	CPSIE I

	BX LR


	.endasmfunc

	; end of the asm file
	.align
	.end
