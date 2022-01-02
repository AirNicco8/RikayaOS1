#include "handler.h"
#include "types_rikaya.h"
#include "p1.5test_rikaya_v0.h"

state_t *oldProcState;

extern pcb_t *currProc;
extern struct list_head ready_queue;

/* Chiamata quando avviene una system call, controlla se si tratta di una system
 * call o di una eccezione, controlla se l'eccezione � la numero tre ed infine
 * termina il processo e torna allo scheduler
 */

void sysCallHandler()
{
	oldProcState = (state_t*) (memaddr)SYSCALL_OLDAREA;

	if (CAUSE_GET_EXCCODE(oldProcState->cause) == EXC_SYS)
	{
		switch (oldProcState->reg_a0)
		{
			/*TODO other SYSCALLs*/
			case SYS(3):
				freePcb(currProc);
				currProc = NULL;
				break;
			default:
				/*TODO log di errore*/
				break;
		}
	}
	else
	{
		/*TODO BP code*/
	}

	scheduler();
}

void trapHandler()
{
	/*TODO*/
}

void tlbHandler()
{
	/*TODO*/
}

/* Viene chiamata quando si trova un interrupt, dentro la funzione si entra in
	kernel mode e ci si assicura che sia presente un local timer
 che viene timer viene risettato (in questo caso al valore massimo),
 lo stato del processo viene salvato in una old area*/

void intHandler()
{
	setSTATUS((getSTATUS() & KM_ON) | LOCTIM_ON);
	setTIMER(0xFFFFFFFF);
	insertProcQ(&ready_queue, currProc);
	oldProcState = (state_t*)INTERRUPT_OLDAREA;

	currProc->p_s.cause = oldProcState->cause;
	currProc->p_s.entry_hi = oldProcState->entry_hi;
	currProc->p_s.hi = oldProcState->hi;
	currProc->p_s.lo = oldProcState->lo;
	currProc->p_s.status = oldProcState->status;

	for (int i = 0; i < STATE_GPR_LEN ; i++)
		currProc->p_s.gpr[i] = oldProcState->gpr[i];

	currProc->p_s.pc_epc = oldProcState->pc_epc;

	scheduler();
}
