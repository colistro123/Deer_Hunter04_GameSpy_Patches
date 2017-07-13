#include "stdafx.h"
#include "GameWindowRelated.h"
#include "Hooking.h"
#include "WindowHooks.h"

RsGlobalType* RsGlobal;

void doWindowGlobals()
{
	RsGlobal = (RsGlobalType*)(VC_BASEADR + 0x5B38E0);
	InstallWindowsHooks();
}