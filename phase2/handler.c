#include "libumps.h"
#include "handler.h"
#include "asl.h"
#include "utils.h"
#include "types_rikaya_new.h"
#include "listx.h"

state_t *oldProcState;

U32 timeout[3];

devregarea_t *dev_base = (devregarea_t *) 0x10000000;

semdev semdInt;

semd_t waitClockList;

state_t *sysBp[2], *tlb[2],* prgTrap[2];

extern pcb_t *currProc;
extern struct list_head ready_queue;

/* Chiamata quando avviene una system call, controlla se si tratta di una system
 * call o di una eccezione, controlla se l'eccezione � la numero tre ed infine
 * termina il processo e torna allo scheduler
 */

void sysCallHandler()
{
	setSTATUS((getSTATUS() | STATUS_IM(1)) & KM_ON);
	oldProcState = (state_t*) (memaddr)SYSCALL_OLDAREA;
	oldProcState->pc_epc += WORD_SIZE;
	if (CAUSE_GET_EXCCODE(oldProcState->cause) == EXC_SYS)
	{
		U64 kmExecTimeTMP;
		switch (oldProcState->reg_a0)
		{
			/*
				Get_CPU_Time, restituisce in 3 variabili separate
				tempo passato dal processo in user, in kernel ed
				il tempo totale passato dalla prima attivazione
			*/
			case SYS(1): /*NON BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				timeout[0] = (unsigned int) (currProc -> exec_time[0] - currProc -> exec_time[1]);
				timeout[1] = (unsigned int) currProc -> exec_time[1];
				timeout[2] = (unsigned int) currProc -> exec_time[2];
				oldProcState->reg_a1 =(unsigned int) &timeout[0];
				oldProcState->reg_a2 =(unsigned int) &timeout[1];
				oldProcState->reg_a3 =(unsigned int) &timeout[2];
				currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
				LDST(oldProcState);
				break;
			/* Create process */ 
			case SYS(2):  /*NON BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				pcb_t new;
				cpyState(&new.p_s,(state_t *) oldProcState->reg_a1);
				new.p_parent = currProc;
				list_add_tail(&currProc->p_child, &new.p_child);
				new.priority = (unsigned int)&oldProcState->reg_a2;
				new.original_priority = new.priority;
				new.exec_time[0] = dev_base->todlo;
				new.exec_time[1] = 0;
				new.exec_time[2] = 0;
				/*oldProcState->reg_v0 = (U32) -1;*/
				if ((void*) oldProcState->reg_a3 != (void*) NULL)
				{
					oldProcState->reg_a3 = (U32) &new;
					oldProcState->reg_a0 = (U32) 0;	
				}
				currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
				LDST(oldProcState);
 				break;
			/* terminate process */
			case SYS(3):  /*NON BLOCCANTE*/
				if ((oldProcState != NULL) && ( descends(currProc, (pcb_t *) oldProcState->reg_a1)  == 1))
				{
					struct list_head *iterator;
					list_for_each(iterator, &(container_of(((pcb_t *) oldProcState->reg_a1)->p_child.next,pcb_t, p_child)->p_sib))
						if (setTutor(container_of(iterator,pcb_t,p_sib)) == 0)
							container_of(iterator, pcb_t, p_sib)->p_parent = godfather(currProc);
					freePcb(currProc);
					oldProcState->reg_a0 = (U32) 0;
					LDST(oldProcState);
				}
				else
				{
					oldProcState->reg_a0 = -1;
					LDST(oldProcState);
				}
				break;
			/* Verhogen */
			case SYS(4):  /*NON BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				if (*((int *)oldProcState->reg_a1) > 0)
				{
					(*((int *) oldProcState->reg_a1))++;
					if (getSemd((int *)oldProcState->reg_a1) == NULL)
					{
						pcb_t *unlocked = removeBlocked((int *)oldProcState->reg_a1);
						/*add unlocked to ready queue*/
						insertProcQ(&ready_queue, unlocked);
					}
					currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
					LDST(oldProcState);
					break;
				}
				else
				{
					(*((int *) oldProcState->reg_a1))++;
					currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
					LDST(oldProcState);
					break;
				}
			/* Passeren */
			case SYS(5): /*BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				if (*((int *)oldProcState->reg_a1) < 1)
				{
					(*((int *)oldProcState->reg_a1))--;
					insertBlocked((int *)oldProcState->reg_a1, currProc);
					currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
					currProc->exec_time[1] += (int) (TIME_SLICE - dev_base->intervaltimer);
					STST(&currProc->p_s);
					currProc = NULL;
					break;
				}
				else
				{
					(*((int *)oldProcState->reg_a1))--;
					currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
					LDST(oldProcState);
					break;
				}
			case SYS(6): /*wait clock*/  /*BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				list_add(&currProc->s_list,&waitClockList.s_procQ);
				currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
				currProc->exec_time[1] += (int) (TIME_SLICE - dev_base->intervaltimer);
				STST(&currProc->p_s);
				currProc -> p_s.pc_epc = oldProcState->pc_epc;
				currProc = NULL;
				break;
			case SYS(7): /*do io*/  /*BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				int line =  getLine(oldProcState->reg_a2);
				int dev = getDev(oldProcState->reg_a2);
				if ((line == 7) && (((oldProcState->reg_a2 + 0x00000380) % 16) > 7))
					line = 8;	/* transmit command */ 
				switch (line)
				{
					case 3:
						if (!list_empty(&semdInt.disk[dev].s_procQ))
							dev_base->devreg[3][0].dtp.command = oldProcState->reg_a1;
						list_add_tail(&currProc->s_list,&semdInt.disk[dev].s_procQ);
						break;
					case 4:
						if (!list_empty(&semdInt.tape[dev].s_procQ))
							dev_base->devreg[1][0].dtp.command = oldProcState->reg_a1;
						list_add_tail(&currProc->s_list,&semdInt.tape[dev].s_procQ);
						break;
					case 5:
						if (!list_empty(&semdInt.network[dev].s_procQ))
							dev_base->devreg[2][0].dtp.command = oldProcState->reg_a1;
						list_add_tail(&currProc->s_list,&semdInt.network[dev].s_procQ);
						break;
					case 6:
						if (!list_empty(&semdInt.printer[dev].s_procQ))
							dev_base->devreg[3][0].dtp.command = oldProcState->reg_a1;
						list_add_tail(&currProc->s_list,&semdInt.printer[dev].s_procQ);
						break;
					case 7:
						if (list_empty(&semdInt.terminalR[dev].s_procQ))
							dev_base->devreg[4][0].term.recv_command = oldProcState->reg_a1;
						list_add_tail(&currProc->s_list,&semdInt.terminalR[dev].s_procQ);
						break;
					case 8:
						if (list_empty(&semdInt.terminalT[dev].s_procQ))
							dev_base->devreg[4][0].term.transm_command = oldProcState->reg_a1;
						list_add_tail(&currProc->s_list,&semdInt.terminalT[dev].s_procQ);
						break;
					default:
						break;	/*do nothing*/
				}
				currProc->p_s.reg_a0 =  dev_base->devreg[line-4][dev].term.transm_status;
				currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
				currProc->exec_time[1] += (int) (TIME_SLICE - dev_base->intervaltimer);
				setSTATUS(getSTATUS()&KM_ON);
				STST(&currProc->p_s);
				currProc->p_s.pc_epc = oldProcState->pc_epc;
				currProc = NULL;
				break;
			/* set tutor */
			case SYS(8):  /*NON BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				if (!emptyChild(currProc))
					setTutorRec(currProc, container_of(&currProc->p_child, pcb_t,p_child));
				currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
				LDST(oldProcState);
				break;
			/* specpassup */
			case SYS(9):  /*NON BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				switch (oldProcState->reg_a1)
				{
					case 0:
						if (sysBp[0] != NULL)
							break;
						sysBp[0] = (state_t *) oldProcState->reg_a2;
						sysBp[1] = (state_t *) oldProcState->reg_a3;
						oldProcState->reg_a0 = 0;
						break;
					case 1:
						if (tlb[0] != NULL)
							break;
						tlb[0] = (state_t *) oldProcState->reg_a2;
						tlb[1] = (state_t *) oldProcState->reg_a3;
						oldProcState->reg_a0 = 0;
						break;
					case 2:
						if (prgTrap[0] != NULL)
							break;
						prgTrap[0] = (state_t *) oldProcState->reg_a2;
						prgTrap[1] = (state_t *) oldProcState->reg_a3;
						oldProcState->reg_a0 = 0;
						break;
					default:
						oldProcState->reg_a0 = -1;
						break;
				}
				currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
				LDST(oldProcState);
				break;
			/* get pid */
			case SYS(10):  /*NON BLOCCANTE*/
				kmExecTimeTMP = *((U64 *) (RAMBASE + 24));
				if((void *) oldProcState->reg_a1 != (void *) NULL)
					oldProcState->reg_a1 = (unsigned int) &currProc;
				if(((void *) oldProcState->reg_a2 != (void *) NULL) && ((void *) currProc->p_parent != (void *) NULL))
					oldProcState->reg_a2 = (unsigned int) &(currProc->p_parent);
				currProc->exec_time[2] += *((U64 *) (RAMBASE + 24)) - kmExecTimeTMP;
				LDST(oldProcState);
				break;
			default:
				if (sysBp[0] == NULL)
				{
					struct list_head* iterator;
					list_for_each(iterator, &(container_of(((pcb_t *) oldProcState->reg_a1)->p_child.next, pcb_t, p_child) -> p_sib))
						if (setTutor(container_of(iterator, pcb_t, p_sib)) == 0)
							container_of(iterator, pcb_t, p_sib)->p_parent = godfather(currProc);
					freePcb(currProc);
					currProc = NULL;
					break;
				} 
				else
				{
					cpyState(sysBp[0],oldProcState);
					LDST(sysBp[1]);
					break;
				}
		}
	}
	else
	{
		/*TODO BP code*/
	}

	scheduler();
}

/* Segnaposto per delle funzioni che non vengono implementate in questa versione. */

void trapHandler()
{
	if (prgTrap[0] == NULL)
		LDST(oldProcState);
	cpyState(prgTrap[0],oldProcState);
	LDST(prgTrap[1]);
}

void tlbHandler()
{
	if ((void *)tlb[0])
		LDST(oldProcState);
	cpyState(tlb[0],oldProcState);
	LDST((void *) tlb[1]);
}

/* Viene chiamata quando si trova un interrupt, dentro la funzione si entra in
	kernel mode e ci si assicura che sia presente un local timer
 che viene timer viene risettato (in questo caso al valore massimo),
 lo stato del processo viene salvato in una old area*/

void intHandler()
{
	setSTATUS((getSTATUS() & KM_ON) | LOCTIM_ON);
	int i;
	oldProcState = (state_t*) (memaddr)INTERRUPT_OLDAREA;
	if (oldProcState->cause & BIT_ON(8)) /*inter processor*/
	{
		/*this shouldn't happen*/
	}
	if (oldProcState->cause & BIT_ON(9)) /*proc local timer*/
	{
		/*activate all processes which called sys6 since last tick,
		  then set the timer to run for 100ms*/
		struct list_head *iterator;
		setTIMER((U32) (100000 / dev_base->timescale));
		list_for_each(iterator, &waitClockList.s_procQ)
		{
			insertProcQ(&ready_queue, container_of(iterator, pcb_t, s_list));
			list_del(iterator);
		}
	}
	if (oldProcState->cause & BIT_ON(10)) /*interval timer*/
	{
		dev_base->intervaltimer = 0xFFFFFFFF;
		insertProcQ(&ready_queue, currProc);
	}

	/*
		per motivi di testing è implementata correttamente solo l'handler
		dei terminal
	*/

	if (oldProcState->cause & BIT_ON(11)) /*disks*/
		for (i=0;i<8;i++)
			if (*IDBM(0) & BIT_ON(i))
			{
				/*remove head of waiting procs, add pcb_t to readyQ*/
				if (dev_base->devreg[0][i].dtp.status != DEV_READY)
				{
					/*TODO errore*/
				}
				else 
				{ 
					if (list_empty(&semdInt.disk[i].s_procQ))
						dev_base->devreg[0][i].dtp.command = ACK;
					else
					{
						while (dev_base->devreg[0][i].dtp.status != DEV_READY);
						dev_base->devreg[0][i].dtp.command = container_of(semdInt.disk[i].s_procQ.next,pcb_t,s_list)->p_s.reg_a2;
					}
				}
			}
	if (oldProcState->cause & BIT_ON(12)) /*tapes*/
	{
		for (i=0;i<8;i++)
			if (*IDBM(1) & BIT_ON(i))
				{
					/*remove head of waiting procs, add pcb_t to readyQ*/
					if (dev_base->devreg[1][i].dtp.status != DEV_READY)
					{
						/*TODO errore*/
					}
					else 
					{ 
						if (list_empty(&semdInt.tape[i].s_procQ))
							dev_base->devreg[1][i].dtp.command = ACK;
						else
						{
							while (dev_base->devreg[1][i].dtp.status != DEV_READY);
							dev_base->devreg[1][i].dtp.command = container_of(semdInt.tape[i].s_procQ.next,pcb_t,s_list)->p_s.reg_a2;
						}
					}
				}
	}
	if (oldProcState->cause & BIT_ON(13)) /*network*/
	{
		for (i=0;i<8;i++)
			if (*IDBM(2) & BIT_ON(i))
			{
				/*remove head of waiting procs, add pcb_t to readyQ*/
				if (dev_base->devreg[2][i].dtp.status != DEV_READY)
				{
					/*TODO errore*/
				}
				else 
				{ 
					if (list_empty(&semdInt.network[i].s_procQ))
						dev_base->devreg[2][i].dtp.command = ACK;
					else
					{
						while (dev_base->devreg[2][i].dtp.status != DEV_READY);
						dev_base->devreg[2][i].dtp.command = container_of(semdInt.network[i].s_procQ.next,pcb_t,s_list)->p_s.reg_a2;
					}
				}
			}
	}
	if (oldProcState->cause & BIT_ON(14)) /*printers*/
	{
		for (i=0;i<8;i++)
			if (*IDBM(3) & BIT_ON(i))
			{
				/*remove head of waiting procs, add pcb_t to readyQ*/
				if (dev_base->devreg[3][i].dtp.status != DEV_READY)
				{
					/*TODO errore*/
				}
				else 
				{ 
					if (list_empty(&semdInt.printer[i].s_procQ))
						dev_base->devreg[3][i].dtp.command = ACK;
					else
					{
						while (dev_base->devreg[3][i].dtp.status != DEV_READY);
						dev_base->devreg[3][i].dtp.command = container_of(semdInt.printer[i].s_procQ.next,pcb_t,s_list)->p_s.reg_a2;
					}
				}
			}
	}
	if (oldProcState->cause & BIT_ON(15)) /*terminals*/
	{
		for (i=0;i<8;i++)
			if (*IDBM(4) & BIT_ON(i))
			{
				if (list_empty(&semdInt.terminalR[i].s_procQ))
					dev_base->devreg[4][i].term.recv_command = ACK;
				else
				{
					dev_base->devreg[4][i].term.recv_command = container_of(semdInt.terminalR[i].s_procQ.next,pcb_t,s_list)->p_s.reg_a2;
					insertProcQ(&ready_queue, container_of(semdInt.terminalR[i].s_procQ.next, pcb_t, s_list));
					list_del(semdInt.terminalR[i].s_procQ.next);
				}
				if (list_empty(&semdInt.terminalT[i].s_procQ))
					dev_base->devreg[4][i].term.transm_command = ACK;
				else
				{
					dev_base->devreg[4][i].term.transm_command = container_of(semdInt.terminalT[i].s_procQ.next,pcb_t,s_list)->p_s.reg_a2;
					insertProcQ(&ready_queue, container_of(semdInt.terminalT[i].s_procQ.next, pcb_t, s_list));
					list_del(semdInt.terminalT[i].s_procQ.next);
				}
			}
	}
	
	cpyState(&currProc->p_s, oldProcState);
	
	scheduler();
}
