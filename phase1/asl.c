#include "listx.h"
#include "const.h"
#include "asl.h"
#include "types_rikaya.h"
#include "pcb.h"

semd_t semd_table[MAXPROC];
struct list_head semdFree_h;
struct list_head semd_h;

/*
	Funzione utilizzata per inizializzare tutti i campi di un semd_t nulli.
*/

static void setEmpty(semd_t *semd)
{
	semd->s_next.prev = NULL;
	semd->s_next.next = NULL;
	semd->s_key = NULL;
	semd->s_procQ.prev = NULL;
	semd->s_procQ.next = NULL;
}

/*
	Restituisce il semaforo appartenente a semd_h di chiave key,
	restituisce NULL se non trova corrispondenze tra gli elementi
	della lista
*/

semd_t* getSemd(int* key)
{
	struct list_head* tmp;
	if (list_empty(&semd_h)) return NULL;
	else
	{
		list_for_each(tmp, &semd_h)
			if(container_of(tmp, semd_t, s_next)->s_key == key)
				return container_of(tmp, semd_t, s_next);
		return NULL;
	}
}

/*
	Inserisce p nella lista semd_h con rispettiva chiave key,
	se manca un semaforo di chiave key e non ce ne sono disponibili
	da allocare restituisce TRUE, altrimenti restituisce TRUE.
	Se necessario (semaforo non presente in semd_h) alloca un semaforo
	da semdFree_h.
*/

int insertBlocked(int *key, pcb_t *p)
{
	semd_t *tmp = getSemd(key);
	if(tmp == NULL)
	{
		if(list_empty(&semdFree_h))
			return TRUE;
		else
		{
			tmp = container_of(semdFree_h.next, semd_t, s_next);
			list_del(&tmp->s_next);
			list_add_tail(&tmp->s_next, &semd_h);
			tmp->s_key = key;
			p->p_semkey = key;
			mkEmptyProcQ(&tmp->s_procQ);
			insertProcQ(&tmp->s_procQ, p);
			return FALSE;
		}
	}
	else
	{
		insertProcQ(&tmp->s_procQ, p);
		return FALSE;
	}
}

/*
        Ritorna il primo PCB della coda processi bloccati associata al descrittore con chiave
        key, se non esiste ritorna NULL, se la coda processi bloccati diventa vuota il
        descrittore sarà tolto dalla ASL e inserito nella lista dei semafori liberi.
*/

pcb_t* removeBlocked(int *key)
{
  semd_t *semd_tmp = getSemd(key);
  if (semd_tmp == NULL)
  	return NULL;
  else
  {
		pcb_t *pcb_tmp = removeProcQ(&semd_tmp->s_procQ);
		if (list_empty(&semd_tmp->s_procQ))
		{
			list_del(&semd_tmp->s_next);
			list_add(&semd_tmp->s_next, &semdFree_h);
		}
		return pcb_tmp;
	}
}

/*
	Rimuove e restituisce p dalla coda del semaforo a cui è assegnato,
	restituisce NULL se non appartiene alla coda del semaforo.
*/

pcb_t *outBlocked(pcb_t *p)
{
	semd_t *semd_tmp = getSemd(p->p_semkey);
	if (semd_tmp == NULL)
		return NULL;
	else
		return outProcQ(&semd_tmp->s_procQ, p);
}

/*
	Restituisce il primo processo nella coda associata al semaforo di chiave key,
	restiuisce NULL se questa è vuota o se il semaforo non appartiene all'ASL.
*/

pcb_t *headBlocked(int *key)
{
	semd_t *semd_tmp = getSemd(key);
        if (semd_tmp == NULL)
                return NULL;
	else
		return (list_empty(semd_tmp->s_procQ.next->prev)) ?
			NULL :
			container_of(semd_tmp->s_procQ.next, pcb_t, p_next);
}

/*
	Elimina ricorsivamente p e tutti i processi che hanno p come avo
	da eventuali semafori a cui appartengono.
*/

void outChildBlocked(pcb_t *p)
{
	outBlocked(p);
	while (p->p_child.next != NULL)
	{
		p = container_of(p->p_child.next, pcb_t, p_child);
		while (p->p_sib.next != NULL)
		{
			outChildBlocked(p);
			p = container_of(p->p_sib.next, pcb_t, p_sib);
		}
		outChildBlocked(p);
	}
}

/*
	Inizializza la lista semdFree_h (semafori liberi) popolandola
	con gli elementi in semd_table
*/

void initASL() {
	INIT_LIST_HEAD(&semdFree_h);
	INIT_LIST_HEAD(&semd_h);
	for (int j = 0; j < MAXPROC; j++)
		list_add(&semd_table[j++].s_next, &semdFree_h);
}
