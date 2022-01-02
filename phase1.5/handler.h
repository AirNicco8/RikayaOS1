#ifndef HANDLER_H
#define HANDLER_H

#include "const.h"
#include "listx.h"
#include "pcb.h"
#include "scheduler.h"
#include "types.h"
#include "types_rikaya.h"

void sysCallHandler();
void trapHandler();
void tlbHandler();
void intHandler();

#endif
