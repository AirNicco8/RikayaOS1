#include "asl.h"
#include "const.h"
#include "cp0.h"
#include "handler.h"
#include "scheduler.h"
#include "types_rikaya_new.h"
#include "utils.h"
#include "p2test.h"

int procCount;
int sftBlkCount;
pcb_t *currProc;
struct list_head ready_queue;

extern semdev semdInt;
extern devregarea_t *dev_base;
/* Setta lo status e attiva interrupt, kernel mode, il processor local timer e
 *  setta la priorità */

void initProc(pcb_t *in)
{
	in->p_s.status = ALL_OFF;
	in->p_s.status |= STATUS_IM(1);
	in->p_s.status |= STATUS_IEo;
	in->p_s.status |= STATUS_IEp;
	in->p_s.status |= STATUS_TE;
	in->p_s.status &= KM_ON;
	in->p_s.reg_sp = RAMTOP - FRAMESIZE;
	in->priority = 1;
	in->original_priority = 1;
	in->exec_time[0] = dev_base->todlo; /* salvo solo la parte bassa, tanto dovrò restituire un u int*/
	in->exec_time[1] = 0;
	in->exec_time[2] = 0;
}

HIDDEN void setPC(pcb_t *in, void (*testn))
{
	in->p_s.pc_epc = (memaddr) testn;
}


/* Inizializza i Pcb ed inserisce i processi nella ready queue*/

int main()
{
	initNewArea(SYSCALL_NEWAREA, sysCallHandler);
	initNewArea(PROGRAMTRAP_NEWAREA, trapHandler);
	initNewArea(TLB_NEWAREA, tlbHandler);
	initNewArea(INTERRUPT_NEWAREA, intHandler);

	initPcbs();
	INIT_LIST_HEAD(&ready_queue);

	initASL();
	initSEMDS(&semdInt);
	pcb_t* _test;

	_test = allocPcb();
	initProc(_test);
	setPC(_test, (void *) test);
	insertProcQ(&ready_queue, _test);

	scheduler();
	
	return 0;
}
