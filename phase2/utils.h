#ifndef UTILS_H
#define UTILS_H
#include "types_rikaya_new.h"


int areProcsWaiting();
void initSEMDS(semdev *in);
void cpyState(state_t *new, state_t *old);
void initNewArea(memaddr newArea, void (*handler));
void setTutorRec(pcb_t* tutor, pcb_t* firstChild);
int descends(pcb_t *parent, pcb_t *in);
int setTutor(pcb_t *child);
pcb_t * godfather(pcb_t *son);
int getLine(U32 in);
int getDev(U32 in);


#endif
