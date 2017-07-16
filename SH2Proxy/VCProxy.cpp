// proxydll.cpp
#include "stdafx.h"
#include "VCProxy.h"
#include "VCPatcher.h"
#include "HookFunction.h"
#include "Hooking.h"
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

// global variables
#pragma data_seg (".d3d8_shared")
myIDirect3DDevice8* gl_pmyIDirect3DDevice8;
myIDirect3D8*       gl_pmyIDirect3D8;
HINSTANCE           gl_hOriginalDll;
HINSTANCE           gl_hThisInstance;
VCPatcher			gl_patcher;
#pragma data_seg ()

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	// to avoid compiler lvl4 warnings 
	LPVOID lpDummy = lpReserved;
	lpDummy = NULL;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: InitInstance(hModule); break;
	case DLL_PROCESS_DETACH: ExitInstance(); break;

	case DLL_THREAD_ATTACH:  break;
	case DLL_THREAD_DETACH:  break;
	}
	return TRUE;
}

// Exported function (faking d3d8.dll's one-and-only export)
IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion)
{
	if (!gl_hOriginalDll) LoadOriginalDll(); // looking for the "right d3d8.dll"

											 // Hooking IDirect3D Object from Original Library
	typedef IDirect3D8 *(WINAPI* D3D8_Type)(UINT SDKVersion);
	D3D8_Type D3DCreate8_fn = (D3D8_Type)GetProcAddress(gl_hOriginalDll, "Direct3DCreate8");

	// Debug
	if (!D3DCreate8_fn)
	{
		OutputDebugString("PROXYDLL: Pointer to original D3DCreate8 function not received ERROR ****\r\n");
		::ExitProcess(0); // exit the hard way
	}

	// Request pointer from Original Dll. 
	IDirect3D8 *pIDirect3D8_orig = D3DCreate8_fn(SDKVersion);

	// Create my IDirect3D8 object and store pointer to original object there.
	// note: the object will delete itself once Ref count is zero (similar to COM objects)
	gl_pmyIDirect3D8 = new myIDirect3D8(pIDirect3D8_orig);

	//gl_patcher.PatchResolution(NULL);
	// Return pointer to hooking Object instead of "real one"
	return (gl_pmyIDirect3D8);
}

#include "GLHook.h"
/* Hooked API Functions*/

DWORD CreateWindowExaddr = 0;				//Global DWORD(4-bytes) to store the address of the CreateWindowEx API
DWORD SetWindowPosExaddr = 0;
DWORD ChangeDisplaySettingsaddr = 0;		//Global DWORD to store the addres of the ChangeDisplaySettings API
DWORD ShowWindowExAddr = 0;

BYTE backupShowWindow[6];
BYTE backupSWP[6];						//Array of bytes to save the original code when we hook the CreateWindowEx API
BYTE backupCW[6];						//Array of bytes to save the original code when we hook the CreateWindowEx API
BYTE backupCDS[6];						//Array of Bytes to save the original code when we hook ChangeDisplaySettings

BOOL WINAPI hook_ShowWindowEx(HWND hWnd, int nCmdShow) {
	//hWndInsertAfter = HWND_BOTTOM;
	nCmdShow = 5;
	WriteProcessMemory(GetCurrentProcess(), (void*)ShowWindowExAddr, backupShowWindow, 6, 0);

	BOOL wndRet = ShowWindow(hWnd, nCmdShow);

	ShowWindowExAddr = HookGeneralFunction("user32.dll", "SetWindowPosExA", hook_ShowWindowEx, backupShowWindow);
	return wndRet;
}

LONG WINAPI hook_ChangeDisplaySettings(LPDEVMODE lpDevMode, DWORD dwflags)
{
	return 0;														//DISP_CHANGE_SUCCESSFUL = 0, We return this value to let the hooked program think that everything went allright
}
BOOL WINAPI hook_SetWindowPosEx(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	hWndInsertAfter = HWND_BOTTOM;
	WriteProcessMemory(GetCurrentProcess(), (void*)SetWindowPosExaddr, backupSWP, 6, 0);

	BOOL wndRet = SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags | SWP_HIDEWINDOW);

	SetWindowPosExaddr = HookGeneralFunction("user32.dll", "SetWindowPosExA", hook_SetWindowPosEx, backupSWP);
	return wndRet;
}

HWND WINAPI hook_CreateWindowEx(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth,
	int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
	dwStyle = WS_MINIMIZEBOX;							// Windows Style
	lpWindowName = "Hooked!";									// We change the window name into "Hooked!" just for fun
	nWidth = 500;											// We change the width, because we don't want a full screen window
	nHeight = 700;											// We change the heigth
	ShowCursor(TRUE);													// Show Mouse Pointer
	x = 0;
	y = 0;
																		/* Write the original code back(Unhook the API) */
	WriteProcessMemory(GetCurrentProcess(), (void*)CreateWindowExaddr, backupCW, 6, 0);

	/* Call the API but with the nessecary changes */
	HWND ret = CreateWindowEx(dwExStyle,						// Extended Style For The Window
		lpClassName,						// Class Name
		lpWindowName,							// Window Title
		dwStyle |							// Defined Window Style
		WS_CLIPSIBLINGS |					// Required Window Style
		WS_CLIPCHILDREN,					// Defined Window Style
		x,
		y,									// Window Position
		nWidth,								// Window Width
		nHeight,								// Window Height
		hWndParent,							// Parent Window
		hMenu,								// Menu
		hInstance,							// Instance
		lpParam);							// Pass Anything To WM_CREATE

	ShowWindow(ret, SW_HIDE);
											/* Reinstall the Hook */
	CreateWindowExaddr = HookGeneralFunction("user32.dll", "CreateWindowExA", hook_CreateWindowEx, backupCW);
	return ret;													// Return the Handle to the window we created
}

static bool returntrue() {
	return false;
}

bool bDelay;
DWORD WINAPI Init(LPVOID)
{
#ifdef _WEIRD_STEAM_VC_BINDING
	auto pattern = hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00");
	}

	if (bDelay)
#endif
	{
		//Ready to go
		//CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
#ifdef COMPILING_2005

		HMODULE hAspen = GetModuleHandle("Aspen.dll");
		if (hAspen)
		{
#if 0
			auto matches = hook::module_pattern(hAspen, "F7 D9 1B C9 83 E1 04 0B CE").count(2);
			for (int i = 0; i < matches.size(); i++)
			{
				//Patch out the weird flag stuff this game does
				char *location = matches.get(i).get<char>(0);
				hook::nopVP(location, 7);
			}

			//F7 D9 1B C9 83 E1 04 0B CE
			auto pattern = hook::module_pattern(hAspen, "C7 44 24 10 40").count(1);
			if (pattern.size() == 1) {
				char *location = pattern.get(0).get<char>(4);
				hook::putVP<uint8_t>(location, 32);
			}
			//hook::nopVP(0x100D4909, 0x19);
			//hook::vp::jump(0x100D49A7, Direct3DCreate8);
#endif
#if _DEBUG
			SetWindowPosExaddr = HookGeneralFunction("user32.dll", "SetWindowPos", hook_SetWindowPosEx, backupSWP);
			CreateWindowExaddr = HookGeneralFunction("user32.dll", "CreateWindowExA", hook_CreateWindowEx, backupCW);
			ChangeDisplaySettingsaddr = HookGeneralFunction("user32.dll", "ChangeDisplaySettingsA", hook_ChangeDisplaySettings, backupCDS);
			ShowWindowExAddr = HookGeneralFunction("user32.dll", "ShowWindow", hook_ShowWindowEx, backupShowWindow);
#endif
			/*
			pattern = hook::module_pattern(hAspen, "E8 ? ? ? ? 85 C0 75 30").count(1);
			if (pattern.size() == 1) {
				//100D43B0
				char *location = pattern.get(0).get<char>(0);
				hook::vp::jump(location, returntrue);
			}
			*/
		}

		
#endif
		HookFunction::RunAll();
		gl_patcher.Init();
	}
	return 0;
}

void InitInstance(HANDLE hModule)
{
	OutputDebugString("PROXYDLL: InitInstance called.\r\n");

	// Initialisation
	gl_hOriginalDll = NULL;
	gl_hThisInstance = NULL;
	gl_pmyIDirect3D8 = NULL;
	gl_pmyIDirect3DDevice8 = NULL;

	// Storing Instance handle into global var
	gl_hThisInstance = (HINSTANCE)hModule;
	Init(NULL);
}

void LoadOriginalDll(void)
{
	char buffer[MAX_PATH];

	// Getting path to system dir and to d3d8.dll
	::GetSystemDirectory(buffer, MAX_PATH);

	// Append dll name
	strcat_s(buffer, MAX_PATH, "\\d3d8.dll");

	// try to load the system's d3d8.dll, if pointer empty
	if (!gl_hOriginalDll) gl_hOriginalDll = ::LoadLibrary(buffer);

	// Debug
	if (!gl_hOriginalDll)
	{
		OutputDebugString("PROXYDLL: Original d3d8.dll not loaded ERROR ****\r\n");
		::ExitProcess(0); // exit the hard way
	}
}

void ExitInstance()
{
	OutputDebugString("PROXYDLL: ExitInstance called.\r\n");

	// Release the system's d3d8.dll
	if (gl_hOriginalDll)
	{
		::FreeLibrary(gl_hOriginalDll);
		gl_hOriginalDll = NULL;
	}
}

