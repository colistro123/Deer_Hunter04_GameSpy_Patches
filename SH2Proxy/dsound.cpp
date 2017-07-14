#include "stdafx.h"
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>


HINSTANCE           gl_hOriginalDll2;

void LoadOriginalDll2(void)
{
	char buffer[MAX_PATH];

	// Getting path to system dir and to d3d8.dll
	::GetSystemDirectory(buffer, MAX_PATH);

	// Append dll name
	strcat_s(buffer, MAX_PATH, "\\dsound.dll");

	// try to load the system's d3d8.dll, if pointer empty
	if (!gl_hOriginalDll2) gl_hOriginalDll2 = ::LoadLibrary(buffer);

	// Debug
	if (!gl_hOriginalDll2)
	{
		OutputDebugString("PROXYDLL: Original dsound.dll not loaded ERROR ****\r\n");
		::ExitProcess(0); // exit the hard way
	}
}

extern "C" HRESULT DirectSoundCreate8_wrapper(LPCGUID pcGuidDevice, struct IDirectSound8* *ppDS, LPUNKNOWN pUnkOuter) {
	
	if (!gl_hOriginalDll2) LoadOriginalDll2(); // looking for the "right dsound.dll"

											 // Hooking IDirect3D Object from Original Library
	typedef IDirectSound *(WINAPI* DSound_Type)(LPCGUID pcGuidDevice, struct IDirectSound8* *ppDS, LPUNKNOWN pUnkOuter);
	DSound_Type DirectSoundCreate8_fn = (DSound_Type)GetProcAddress(gl_hOriginalDll2, "DirectSoundCreate8");

	// Debug
	if (!DirectSoundCreate8_fn)
	{
		OutputDebugString("PROXYDLL: Pointer to original D3DCreate8 function not received ERROR ****\r\n");
		::ExitProcess(0); // exit the hard way
	}

	// Request pointer from Original Dll. 
	IDirectSound *pIDirect3D8_orig = DirectSoundCreate8_fn(pcGuidDevice, ppDS, pUnkOuter);

	return (HRESULT)(pIDirect3D8_orig);
}