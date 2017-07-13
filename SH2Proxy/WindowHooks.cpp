#include "stdafx.h"
#include "Hooking.h"
#include "GameWindowRelated.h"
#include <Windows.h>
#include <thread>

LONG OldWndProc;
#define MWHEEL_UP	120
#define MWHEEL_DN	-120

void ProcessMWheelEventsUpDown(WPARAM wParam) {
	int zDelta = (short)HIWORD(wParam);
	/*
	if (zDelta == MWHEEL_UP)
		//AudioController__TrySetRadio(GetCurrentRadioID(), true);
	else
		//AudioController__TrySetRadio(GetCurrentRadioID(), false);
	return;
	*/
}
LRESULT CALLBACK NewWndProc(HWND Hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
#if 0
		case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_INSERT:
			{
				//blah
				break;
			}
			}
		}
#endif
		case WM_MOUSEWHEEL:
		{
			//ProcessMWheelEventsUpDown(wParam);
		}
	}
	return CallWindowProc((WNDPROC)OldWndProc, Hwnd, Message, wParam, lParam);
}

void InstallWindowsHooks()
{
	OldWndProc = SetWindowLong(RsGlobal->ps->window, GWL_WNDPROC, (long)NewWndProc);
}