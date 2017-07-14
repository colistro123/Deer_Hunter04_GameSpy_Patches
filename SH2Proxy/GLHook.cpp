#include "stdafx.h""
#include "GLHook.h"
#include <stdio.h>			// Header File For Standard Input/Output 

//this will hook a process and all of it`s modules (loaded DLLs)
//by writting to kernel`s area

/*	This Code will create a process-wide hook by writing to the kernel's	*
*  Parameters Are:															*
*	Dll				- Name of the Dll that contains the API					*
*	FuncName		- Name of the API you want to hook						*
*	Function		- Name of the function the API gets redirected to		*
*	backup			- Array of bytes the original code will be read to		*/

DWORD HookGeneralFunction(const char *Dll, const char *FuncName, void *Function, unsigned char *backup)
{
	DWORD addr = (DWORD)GetProcAddress(GetModuleHandle(Dll), FuncName);					// Get the address of the API
	BYTE jmp[6] = { 0xe9,			//jmp
		0x00, 0x00, 0x00, 0x00,		//address
		0xc3 };						//retn

	ReadProcessMemory(GetCurrentProcess(), (void*)addr, backup, 6, 0);					// Read the first 6 Bytes of the API and save them
	DWORD calc = ((DWORD)Function - addr - 5);											// Calculate the jump
	memcpy(&jmp[1], &calc, 4);															//build the jmp
	WriteProcessMemory(GetCurrentProcess(), (void*)addr, jmp, 6, 0);					//Overwrite the first 6 Bytes of the API with a jump to our function

	return addr;																		//Return the address of the API(so we can restore the original code if needed
}