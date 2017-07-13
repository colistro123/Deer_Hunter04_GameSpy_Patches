#ifndef VCPATCHER_H_
#define VCPATCHER_H_
#include "stdafx.h"

class VCPatcher
{
public:
	bool Init();
	bool PatchResolution(D3DPRESENT_PARAMETERS* pPresentationParameters);
};

#endif