#ifndef UTILS_C
#define UTILS_C

#include "const.h"
#include "types_rikaya.h"
#include "libumps.h"
#include "utils.h"

/* Viene inizializzato il PC all'indirizzo dell'handler del nucleo che gestisce
 l'eccezione, in seguito viene inizializzato il registro sp a RAMTOP e abilita
 il timer*/

void initNewArea(memaddr newArea, void (*handler))
{
	state_t* newLoc;
	newLoc = (state_t*) newArea;
	newLoc->reg_sp = RAMTOP;
	newLoc->status |= LOCTIM_ON;
	newLoc->pc_epc = (memaddr) handler;
}

#endif
