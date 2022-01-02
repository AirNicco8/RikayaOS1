#include "pcb.h"

struct list_head pcbfree_h;
pcb_t pcbFree_table[MAXPROC];

/* Funzione usata per azzerare i campi di un pcb_t */

static pcb_t* setEmpty(pcb_t* pcb)
{
	int i = 0;
	pcb->p_next.next = NULL;
	pcb->p_next.prev = NULL;
	pcb->p_parent = NULL;
	pcb->p_child.next = NULL;
	pcb->p_child.prev = NULL;
	pcb->p_sib.next = NULL;
	pcb->p_sib.prev = NULL;
	pcb->p_s.entry_hi = 0;
	pcb->p_s.cause = 0;
	pcb->p_s.status = 0;
	pcb->p_s.pc_epc = 0;
	while (i < STATE_GPR_LEN)
		pcb->p_s.gpr[i++] = 0;
	pcb->p_s.hi = 0;
	pcb->p_s.lo = 0;
	pcb->priority = 0;
	pcb->original_priority = 0;
	pcb->p_semkey = NULL;
	return pcb;
}

/* Funzione che inizializza la lista pcbfree_h dei pcb liberi, riempiendo questa
 *  col contenuto dell'array pcbFree_table. */

void initPcbs()
{
	INIT_LIST_HEAD(&pcbfree_h);
	int i = 0;
	while (i < MAXPROC)
		list_add(&pcbFree_table[i++].p_next, &pcbfree_h);
}

/* Inserisce p nella lista pcbfree_h */

void freePcb(pcb_t* p) {
	list_add_tail(&p->p_next, &pcbfree_h);
}

/* Controlla che pcbfree_h non sia vuota (nel caso restituisce NULL) altrimenti
 *  rimuove un elemento e lo restituisce con tutti i campi azzerati. */

pcb_t* allocPcb() {
	if (list_empty(&pcbfree_h))
		return NULL;
	else {
		pcb_t* tmp_pcb = container_of(pcbfree_h.next, pcb_t, p_next);
		list_del(&tmp_pcb->p_next);
		return setEmpty(tmp_pcb);
	}
}

/* Inizializza l'elemento sentinella della lista dei PCB */

void mkEmptyProcQ(struct list_head* head) {
	INIT_LIST_HEAD(head);
}

/* Controlla se la lista è vuota, nel caso restituisce TRUE, FALSE altrimenti */

int emptyProcQ(struct list_head* head) {
	return list_empty(head);
}

/* Aggiunge p alla lista dei processi attivi, mantenendo questa ordinata per
 *  priorità */

void insertProcQ(struct list_head* head, pcb_t* p) {
	struct list_head* tmp;
	list_for_each(tmp, head)
	{
		if (container_of(tmp, pcb_t, p_next)->priority <= p->priority)
			break;
	}
	__list_add(&p->p_next, tmp->prev, tmp);
}

/* Restituisce l'elemento di testa della lista dei processi	non rimuovendolo
 *  (NULL se la lista è vuota) */

pcb_t *headProcQ(struct list_head* head) {
	if (emptyProcQ(head))
		return NULL;
	else
		return container_of(head->next, pcb_t, p_next);
}

/* Rimuove il primo elemento dalla coda di processi e lo restituisce (NULL se
 *  lista vuota) */

pcb_t* removeProcQ(struct list_head* head) {
	if (emptyProcQ(head))
		return NULL;
	else
	{
		struct list_head* tmp_lh = head->next;
		list_del(tmp_lh);
		return container_of(tmp_lh, pcb_t, p_next);
	}
}

/* Rimuove p dalla lista dei processi se vi appartiene restituendolo,	altrimenti
 *  (lista vuota e\o p non appartiene) restituisce NULL */

pcb_t* outProcQ(struct list_head* head, pcb_t* p) {
	if (emptyProcQ(head))
		return NULL;
	else
	{
		pcb_t* tmp_pcb = NULL;
		struct list_head* tmp_lh;
		list_for_each(tmp_lh, head)
		{
			if (tmp_lh == &p->p_next)
			{
				tmp_pcb = p;
				list_del(tmp_lh);
				break;
			}
		}
		return tmp_pcb;
	}
}

/* Restituisce TRUE se p non ha figli, FALSE altrimenti */

int emptyChild(pcb_t* p) {
	if (p->p_child.next != NULL)
		return FALSE;
	else
		return TRUE;
}

/* Inserisce p come figlio di prnt e nel farlo aggiorna la lista p_sib */

void insertChild(pcb_t* prnt, pcb_t* p)
{
/*
	if (emptyChild(prnt))
		p->p_sib.next = NULL;
	else
	{
		p->p_sib.next = &container_of(prnt->p_child.next, pcb_t, p_child)->p_sib;
		p->p_sib.next->prev = &p->p_sib;
	}
	p->p_sib.prev = NULL;
	p->p_parent = prnt;
	prnt->p_child.next = &p->p_child;
*/
	if (emptyChild(prnt))
	{
		p->p_sib.prev = NULL;
		prnt->p_child.next = &p->p_child;
	}
	else
	{
		struct list_head *tmp = &container_of(prnt->p_child.next, pcb_t, p_child)->p_sib;
		while (tmp->next != NULL)
			tmp = tmp->next;
		tmp->next = &p->p_sib;
		p->p_sib.prev = tmp;
	}
	p->p_sib.next = NULL;
	p->p_parent = prnt;
}

/* Rimuove il primo figlio di p (aggiornando sia la lista p_child che la lista p_sib),
 *  se p non ha figli restituisce NULL */

pcb_t* removeChild(pcb_t* p) {
	if (emptyChild(p))
		return NULL;
	else
	{
		pcb_t* child = container_of(p->p_child.next, pcb_t, p_child);
		if (child->p_sib.next != NULL)
		{
			p->p_child.next = &container_of(child->p_sib.next, pcb_t, p_sib)->p_child;
			container_of(p->p_child.next, pcb_t, p_child)->p_sib.prev = NULL;
		}
		else
			p->p_child.next = NULL;
		child->p_parent = NULL;
		return child;
	}
}

/* Se p ha un padre, viene rimosso dalla lista dei figli e restituito, se no
 *  (p non ha padre) viene restituito NULL. */

pcb_t* outChild(pcb_t* p) {
	if (p->p_parent == NULL)
		return NULL;
	else
	{
		if (p->p_sib.prev == NULL)
			removeChild(p->p_parent);
		else
		{
			if (p->p_sib.next == NULL)
				p->p_sib.prev->next = NULL;
			else
				list_del(&p->p_sib);
		}
		p->p_parent = NULL;
		return p;
	}
}
