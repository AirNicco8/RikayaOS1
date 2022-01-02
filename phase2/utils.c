#ifndef UTILS_C
#define UTILS_C

#include "pcb.h"
#include "utils.h"
#include "types.h"
#include "libumps.h"
#include "const.h"


extern semdev semdInt;
extern semd_t waitClockList;

/*funzione che controlla se ci sono processi in code per IO*/

int areProcsWaiting()
{
	if(!list_empty(&waitClockList.s_procQ))
		return 1;
	
	for(int i=0; i<DEV_PER_INT;i++)
		if(!list_empty(&semdInt.disk[i].s_procQ))
			return 1;

	for(int i=0; i<DEV_PER_INT;i++)
		if(!list_empty(&semdInt.tape[i].s_procQ))
			return 1;

	for(int i=0; i<DEV_PER_INT;i++)
		if(!list_empty(&semdInt.printer[i].s_procQ))
			return 1;

	for(int i=0; i<DEV_PER_INT;i++)
		if(!list_empty(&semdInt.network[i].s_procQ))
			return 1;

	for(int i=0; i<DEV_PER_INT;i++)
		if(!list_empty(&semdInt.terminalT[i].s_procQ))
			return 1;

	for(int i=0; i<DEV_PER_INT;i++)
		if(!list_empty(&semdInt.terminalR[i].s_procQ))
			return 1;

	return 0;
}

/* funzione che inizializza i semafori dei device */

void initSEMDS(semdev *in)
{
	
	for (int j=0; j<DEV_PER_INT; j++)
		INIT_LIST_HEAD(&in->disk[j].s_procQ);
	for (int j=0; j<DEV_PER_INT; j++)
		INIT_LIST_HEAD(&in->tape[j].s_procQ);
	for (int j=0; j<DEV_PER_INT; j++)
		INIT_LIST_HEAD(&in->network[j].s_procQ);
	for (int j=0; j<DEV_PER_INT; j++)
		INIT_LIST_HEAD(&in->printer[j].s_procQ);
	for (int j=0; j<DEV_PER_INT; j++)
		INIT_LIST_HEAD(&in->terminalR[j].s_procQ);
	for (int j=0; j<DEV_PER_INT; j++)
		INIT_LIST_HEAD(&in->terminalT[j].s_procQ);	
}


/* Funzione che copia i contenuto di uno state_t pointer in un altro state_t pointer  */

void cpyState(state_t *new, state_t*old)
{
	new->entry_hi = old->entry_hi;
	new->cause = old->cause;
	new->status = old->status;
	new->pc_epc = old->pc_epc;
	new->hi = old->hi;
	new->lo = old->lo;

	for(int i=0; i<STATE_GPR_LEN; i++)
		new->gpr[i] = old->gpr[i];
}

/* Inizializza una nuova area */

/* Viene inizializzato il PC all'indirizzo dell'handler del nucleo che gestisce
 l'eccezione, in seguito viene inizializzato il registro sp a RAMTOP e abilita
 il timer*/

void initNewArea(memaddr newArea, void (*handler))
{
	state_t* newLoc = (state_t*) newArea;
	newLoc->reg_sp = RAMTOP;
	newLoc->status |= LOCTIM_ON;
	newLoc->pc_epc = (memaddr) handler;
}

/* Funzione alla quale viene passato un tutor, il quale viene impostato come tutor di tutti i processi figli del processo che viene passato. \*/

void setTutorRec(pcb_t * tutor, pcb_t *first_child) {
	struct list_head *iterator;
	list_for_each(iterator, &(first_child->p_sib)) {
		if (container_of(iterator, pcb_t, p_sib)->p_tutor == NULL)
			container_of(iterator, pcb_t, p_sib)->p_tutor = tutor;
		if (!emptyChild(container_of(iterator, pcb_t, p_sib)))
			setTutorRec (tutor, container_of(container_of(iterator, pcb_t, p_sib)->p_child.next, pcb_t, p_child));
	}
}

/* Riceve due processi come parametri, restituisce 1 se il primo processo è figlio del secondo e 0 altrimenti. */

int descends(pcb_t *parent,pcb_t *in)
{
	if (in == parent) return 1;
	else if (in == NULL) return 0;
	else return descends(parent, in->p_parent);
}

/* Funzione che controlla se il tutor è fissato e, nel caso sia così il tutor viene fatto diventare padre. */

int setTutor(pcb_t *child)
{
	if (child->p_tutor != NULL)
	{
		child->p_parent = child->p_tutor;
		return 1;
	}
	else return 0;
}

/* Funzione che ritorna il parente più lontano, cioè il primo dei padri sopra al quale non c'è nessuno */

pcb_t * godfather(pcb_t *son)
{
	if (son->p_parent == NULL) return son;
	else return godfather(son->p_parent);
}

/* getLine e getDev interpretano con delle maschere i comandi che vengono passati alla sys7 e ne estrapolano rispettivamente la linea e il device coinvolti. */

int getLine(U32 in)
{
	int line = (int) (((in + 0x00000030) & 0x00000380)>>7);
	if ((line > 0) && (line < 6))
		return line+2;
	else return 0;
}

int getDev(U32 in)
{
	int dev = (int) (((in + 0x00000030) & 0x00000070) >> 4);
	return dev;
}

#endif
