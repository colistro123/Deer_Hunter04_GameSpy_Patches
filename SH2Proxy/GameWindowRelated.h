#include <Windows.h>
#define VC_BASEADR 0x400000

// RsGlobalType
struct RsInputDevice
{
	int inputDeviceType;
	bool used;
	void* inputEventHandler;
};

struct PsGlobalType
{
	HWND window;
};

struct RsGlobalType
{
	const char* appName;
	int windowWidth;
	int windowHeight;
	int maximumWidth;
	int maximumHeight;
	int frameLimit;
	bool quit;
	PsGlobalType* ps;
	RsInputDevice keyboard;
	RsInputDevice mouse;
	RsInputDevice pad;
};

extern RsGlobalType* RsGlobal;
void doWindowGlobals();