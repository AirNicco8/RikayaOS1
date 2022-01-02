#include "scheduler.h"
#include "p1.5test_rikaya_v0.h"

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

/*
 * Controlla che ci siano processi da eseguire, quindi rimuove il processo in
 * testa (quello con priorit� pi� alta). In seguito scorre tutti i processi ed
 * aumenta di 1 la priorit� di tutti (tranne che del processo rimosso) e chiama
 * log_process_order; in fine prepara lo status del processore per permetteregli
 * di eseguire i processi: attiva interrupt, attiva kernel mode, attiva il timer
 * e lo setta a 3 ms e poi cede il controllo al processo
 */

void scheduler()
{
	if (!emptyProcQ(&ready_queue) && procCount <= MAXPROC)
	{
		struct list_head *tmp;
		currProc = removeProcQ(&ready_queue);
		log_process_order(currProc->original_priority);
		list_for_each(tmp, &ready_queue)
			container_of(tmp, pcb_t, p_next)->priority++;
		currProc->priority = currProc->original_priority;
		setTIMER(TIME_SLICE);
		LDST(&currProc->p_s);
	}
	else
	{
		HALT();
	}

}
