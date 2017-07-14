#ifndef GLHOOK_H
#define GLHOOK_H
#include <Windows.h>
/* Function Prototype */
DWORD HookGeneralFunction(const char *Dll, const char *FuncName, void *Function, unsigned char *backup);
#endif