#include "const.h"
#include "handler.h"
#include "scheduler.h"
#include "types_rikaya.h"
#include "utils.h"
#include "p1.5test_rikaya_v0.h"

int procCount;
int sftBlkCount;
pcb_t *currProc;
struct list_head ready_queue;

/* Setta lo status e attiva interrupt, kernel mode, il processor local timer e
 *  setta la priorità */

HIDDEN void initProc(pcb_t *in, int n)
{
	in->p_s.status = ALL_OFF;
	in->p_s.status |= STATUS_IM(1);
	in->p_s.status |= STATUS_IEp;
	in->p_s.status |= STATUS_TE;
	in->p_s.status &= KM_ON;
	in->p_s.reg_sp = RAMTOP - (FRAMESIZE * n);
	in->priority = n;
	in->original_priority = n;
}

/* Funzione utilizzata per settare il program counter */

HIDDEN void setPC(pcb_t *in, void (*testn))
{
	in->p_s.pc_epc = (memaddr) testn;
}

/* Inizializza i Pcb ed inserisce i processi nella ready queue*/

int main()
{
	initNewArea(SYSCALL_NEWAREA, sysCallHandler);
	/*
	initNewArea(PROGRAMTRAP_NEWAREA, trapHandler);
	initNewArea(TLB_NEWAREA, tlbHandler);
	*/
	initNewArea(INTERRUPT_NEWAREA, intHandler);

	initPcbs();
	INIT_LIST_HEAD(&ready_queue);

	pcb_t* tests[3];
	void* tests_h[3] = {test1, test2, test3};

	for (int i = 0; i < 3; i++)
	{
		tests[i] = allocPcb();
		initProc(tests[i], i+1);
		setPC(tests[i], tests_h[i]);
		insertProcQ(&ready_queue, tests[i]);
	}

	scheduler();

	return 0;
}
