#include "scheduler.h"
#include "utils.h"
/* global vars for maintaining cpu time usage */
/*
cpu_t TODStarted;
cpu_t currentTOD;
*/

/* external globals scheduler uses */
extern int procCount;
extern int sftBlkCount;

extern pcb_t *currProc;
extern struct list_head ready_queue;
extern devregarea_t *dev_base;


/*
 * Controlla che ci siano processi da eseguire, quindi rimuove il processo in
 * testa (quello con priorita\' piu\' alta). In seguito scorre tutti i processi ed
 * aumenta di 1 la priorita\' di tutti (tranne che del processo rimosso) e chiama
 * log_process_order; in fine prepara lo status del processore per permetteregli
 * di eseguire i processi: attiva interrupt, attiva kernel mode, attiva il timer
 * e lo setta a 3 ms e poi cede il controllo al processo
 */

void scheduler()
{
	if (!emptyProcQ(&ready_queue) && (procCount <= MAXPROC))
	{
		if (currProc != NULL) currProc- exec_time[0] += TIME_SLICE;
		struct list_head *tmp;
		currProc = removeProcQ(&ready_queue);
		if (currProc->exec_time[2] == 0)
			currProc->exec_time[2] = dev_base->todlo; /*<-funzine del tempo*/
		list_for_each(tmp, &ready_queue)
			container_of(tmp, pcb_t, p_next)->priority++;
		currProc->priority = currProc->original_priority;
		dev_base->intervaltimer = TIME_SLICE * dev_base->timescale;
		LDST(&currProc->p_s);
	}
	else
	{
		if (areProcsWaiting())
		{
			setSTATUS((getSTATUS() | STATUS_IM(1)) & KM_ON);
			WAIT();
			scheduler();
		}
		else
			HALT();
	}

}
