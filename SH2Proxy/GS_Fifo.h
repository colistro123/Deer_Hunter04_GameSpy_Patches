#ifndef GS_FIFO_H
#define GS_FIFO_H
#include "GameSpyIncs.h"
static void FIFOAddRear(SBServerFIFO *fifo, SBServer server);
static void FIFOAddFront(SBServerFIFO *fifo, SBServer server);
static SBServer FIFOGetFirst(SBServerFIFO *fifo);
bool FIFORemove(SBServerFIFO *fifo, SBServer server);
static void FIFOClear(SBServerFIFO *fifo);
#endif