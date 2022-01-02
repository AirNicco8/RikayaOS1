/* Header file containing constants */
#ifndef CONST_H
#define CONST_H

#include "arch.h"
#include "cp0.h"

/*HIDDEN/private static/persistent*/
#define HIDDEN static
/*Previous phase consts*/
#define MAXPROC   20
/*number of usermode processes (not including master proc and system daemons)*/
#define UPROCMAX    3
#define	TRUE 	      1
#define	FALSE	      0
#define ON 	        1
#define OFF 	      0
#define EOS         '\0'
#define DEV_PER_INT 8       /* Maximum number of devices per interrupt line */
#define CR          0x0a    /* carriage return as returned by the terminal */

#ifndef NULL
#define NULL ((void *) 0)
#endif

/*Status masks*/
#define INT_ON			0x00000100 /* OR to turn on plt interrupt */
#define INT_OFF			0xFFFFFFFB /* AND to turn off interrupts */
#define INTC_ON			0x00000001 /* OR to turn interrupt current on */
#define INTM_ON			0x0000FF00 /* OR to enable interrupt mask */
#define INTM_OFF		0xFFFF00FF /* AND to turn off interrupt mask */
#define LOCTIM_ON		0x08000000 /* OR to enable local timer */
#define LOCTIM_OFF	0xF7FFFFFF /* AND to turn off local timer */
#define VM_ON			  0x02000000 /* OR to turn on virtual memory */
#define VM_OFF	   	0xFDFFFFFF /* AND to disable virtual memory */
#define KM_ON			  0xFFFFFFF7 /* AND to enable kernel mode*/
#define USER_ON			0x00000008 /* OR to set in user mode */
#define ALL_OFF			0x00000000

/*Bus regs addresses*/
#ifndef RAMBASE
#define RAMBASE         *((unsigned int *)0x10000000)
#endif
#define RAM_INSTALLED   *((unsigned int *)0x10000004)
#define TODLOW		      0x1000001C
#define INTERVAL_TIMER	0x10000020
#define TIMESCALE	      *((unsigned int *)0x10000024)

/*RAM*/
#ifndef RAMTOP
#define RAMTOP          RAMBASE + RAM_INSTALLED
#endif

/*Old/New Areas addresses*/
#define INTERRUPT_OLDAREA 	RAMBASE
#define INTERRUPT_NEWAREA 	0x2000008C
#define TLB_OLDAREA    	  	0x20000118
#define TLB_NEWAREA         0x200001A4
#define PROGRAMTRAP_OLDAREA 0x20000230
#define PROGRAMTRAP_NEWAREA 0x200002BC
#define SYSCALL_OLDAREA     0x20000348
#define SYSCALL_NEWAREA     0x200003D4

/*Cause.ExcCode*/
#define EXC_SYS   8
#define EXC_BP    9

/*SYSCALL System Calls*/
#define SYS(x)  x

/*Time slice for scheduling*/
#define TIME_SLICE		TIMESCALE * 3000U

/*RAMEZIE*/
#define FRAMESIZE 4096

#endif
