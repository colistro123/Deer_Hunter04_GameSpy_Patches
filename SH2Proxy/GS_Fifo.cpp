#include "stdafx.h"
#include "GS_Fifo.h"
#include "gsAssert.h"

#define GSI_MANIC_DEBUG //sure
#ifdef GSI_MANIC_DEBUG
// Make sure the server isn't already in the fifo
void FIFODebugCheckAdd(SBServerFIFO *fifo, SBServer server)
{
	SBServer aServer = fifo->first;
	while (aServer != NULL)
	{
		GS_ASSERT(aServer != server);
		aServer = aServer->next;
	}
}


// Verify the contents of the fifo
void FIFODebugCheck(SBServerFIFO *fifo)
{
	int i = 0;
	SBServer aServer;

	GS_ASSERT(fifo != NULL);
	aServer = fifo->first;
	for (i = 0; i < fifo->count; i++)
	{
		GS_ASSERT(aServer != NULL);
		aServer = aServer->next;
	}
}
#else
#define FIFODebugCheckAdd(a,b)
#define FIFODebugCheck(a)
#endif

//FIFO Queue management functions
static void FIFOAddRear(SBServerFIFO *fifo, SBServer server)
{
	FIFODebugCheckAdd(fifo, server);

	if (fifo->last != NULL)
		fifo->last->next = server;
	fifo->last = server;
	server->next = NULL;
	if (fifo->first == NULL)
		fifo->first = server;
	fifo->count++;

	FIFODebugCheck(fifo);
}

static void FIFOAddFront(SBServerFIFO *fifo, SBServer server)
{
	FIFODebugCheckAdd(fifo, server);

	server->next = fifo->first;
	fifo->first = server;
	if (fifo->last == NULL)
		fifo->last = server;
	fifo->count++;

	FIFODebugCheck(fifo);
}

static SBServer FIFOGetFirst(SBServerFIFO *fifo)
{
	SBServer hold;
	hold = fifo->first;
	if (hold != NULL)
	{
		fifo->first = hold->next;
		if (fifo->first == NULL)
			fifo->last = NULL;
		fifo->count--;
	}

	FIFODebugCheck(fifo);
	return hold;
}

bool FIFORemove(SBServerFIFO *fifo, SBServer server)
{
	SBServer hold, prev;
	prev = NULL;
	hold = fifo->first;
	while (hold != NULL)
	{
		if (hold == server) //found
		{
			if (prev != NULL) //there is a previous..
				prev->next = hold->next;
			if (fifo->first == hold)
				fifo->first = hold->next;
			if (fifo->last == hold)
				fifo->last = prev;
			fifo->count--;
			//	GS_ASSERT((fifo->count == 0 && fifo->first == NULL && fifo->last == NULL) || fifo->count > 0);
			return true;
		}
		prev = hold;
		hold = hold->next;
	}

	FIFODebugCheck(fifo);
	return false;
}

static void FIFOClear(SBServerFIFO *fifo)
{
	fifo->first = fifo->last = NULL;
	fifo->count = 0;

	FIFODebugCheck(fifo);
}