#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "VCPatcher.h"
#include "Hooking.Patterns.h"
#include "Utils.h"
#include <stdio.h>
#include "gsAssert.h"
#include "MinHook.h" //yeah no...
#include "gameOffsets.h"

#define CONSOLE_ENABLED_THAT_CRASHES
#ifdef COMPILING_2005
#define GAME_VERSION_NEEDED "1.2" //1.2 backwards (2.1)
#else
#define GAME_VERSION_NEEDED "1.1" //1.1
#endif

#include <sstream>
#include <iomanip>      // std::setfill, std::setw

PCHAR*
CommandLineToArgvA(
PCHAR CmdLine,
int* _argc
)
{
	PCHAR* argv;
	PCHAR  _argv;
	ULONG   len;
	ULONG   argc;
	CHAR   a;
	ULONG   i, j;

	BOOLEAN  in_QM;
	BOOLEAN  in_TEXT;
	BOOLEAN  in_SPACE;

	len = strlen(CmdLine);
	i = ((len + 2) / 2)*sizeof(PVOID)+sizeof(PVOID);

	argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
		i + (len + 2)*sizeof(CHAR));

	_argv = (PCHAR)(((PUCHAR)argv) + i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = FALSE;
	in_TEXT = FALSE;
	in_SPACE = TRUE;
	i = 0;
	j = 0;

	while (a = CmdLine[i]) {
		if (in_QM) {
			if (a == '\"') {
				in_QM = FALSE;
			}
			else {
				_argv[j] = a;
				j++;
			}
		}
		else {
			switch (a) {
			case '\"':
				in_QM = TRUE;
				in_TEXT = TRUE;
				if (in_SPACE) {
					argv[argc] = _argv + j;
					argc++;
				}
				in_SPACE = FALSE;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if (in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = FALSE;
				in_SPACE = TRUE;
				break;
			default:
				in_TEXT = TRUE;
				if (in_SPACE) {
					argv[argc] = _argv + j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = FALSE;
				break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
}

static float lasttext;
static bool windowShown;
#include "timer.h"
#include <iostream>

void logFuncCustom(void* _thisptr, char* logEntry, ...) {
	FILE* logFile = _wfopen(L"GameMessages.log", L"a");
	if (logFile)
	{
		char bufferOut[1024];
#ifdef CONSOLE_ENABLED_THAT_CRASHES
		char bufferOutConsole[1024];
#endif
		va_list argptr;
		char end_char = logEntry[strlen(logEntry) - 1];
		_snprintf(bufferOut, sizeof(bufferOut), (end_char == '\n') ? "%s" : "%s\n", logEntry);
		va_start(argptr, logEntry);
		vfprintf(logFile, bufferOut, argptr);
#ifdef CONSOLE_ENABLED_THAT_CRASHES
		vsprintf(bufferOutConsole, bufferOut, argptr);
#endif
		va_end(argptr);
		fclose(logFile);
#ifdef CONSOLE_ENABLED_THAT_CRASHES
		printf_s(bufferOutConsole);
#endif
	}
	return;
}

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>
#include "GameSpyIncs.h"

#pragma comment(lib, "ws2_32.lib")


#define MULTIPLIER -1664117991
static SOCKET g_gameSocket;
static int lastReceivedFrom;
static ULONG g_pendSendVar;

static int StringHash(const char *s, int numbuckets)
{
	goa_uint32 hashcode = 0;
	while (*s != 0)
	{
		hashcode = (goa_uint32)((int)hashcode * MULTIPLIER + tolower(*s));
		s++;
	}
	return (int)(hashcode % numbuckets);

}

#define SBOverrideMasterServer FALSE
SBServer SBNullServer = NULL;

int __stdcall CfxBind(SOCKET s, sockaddr * addr, int addrlen)
{
	sockaddr_in* addrIn = (sockaddr_in*)addr;

	printf_s("binder on %i is %p, %p\n", htons(addrIn->sin_port), (void*)s, _ReturnAddress());

	if (htons(addrIn->sin_port) == 34567)
	{
		g_gameSocket = s;
	}

	return bind(s, addr, addrlen);
}

int __stdcall CfxRecvFrom(SOCKET s, char * buf, int len, int flags, sockaddr * from, int * fromlen)
{
	static char buffer[65536];
	uint16_t netID = 0;
	sockaddr_in* outFrom = (sockaddr_in*)from;
	char addr[60];

	inet_ntop(AF_INET, &outFrom->sin_addr.s_addr, addr, sizeof(addr));
	printf_s("CfxRecvFrom (from %i %s) %i bytes on %p, port: %i, %p\n", netID, addr, len, (void*)s, htons(outFrom->sin_port), _ReturnAddress());
	return recvfrom(s, buf, len, flags, from, fromlen);
}

std::string string_to_hex(const std::string& input)
{
	static const char* const lut = "0123456789ABCDEF";
	size_t len = input.length();

	std::string output;
	output.reserve(2 * len);
	for (size_t i = 0; i < len; ++i)
	{
		const unsigned char c = input[i];
		output.push_back(lut[c >> 4]);
		output.push_back(lut[c & 15]);
	}
	return output;
}

int __stdcall CfxSendTo(SOCKET s, char * buf, int len, int flags, sockaddr * to, int tolen)
{
	sockaddr_in* toIn = (sockaddr_in*)to;

	if (s == g_gameSocket)
	{
		if (toIn->sin_addr.S_un.S_un_b.s_b1 == 0xC0 && toIn->sin_addr.S_un.S_un_b.s_b2 == 0xA8)
		{
			g_pendSendVar = 0;

			//if (CoreIsDebuggerPresent())
			{
				printf_s("CfxSendTo (to internal address %i) port: %i, %i b (from thread 0x%x), %p\n", (htonl(toIn->sin_addr.s_addr) & 0xFFFF) ^ 0xFEED, htons(toIn->sin_port), len, GetCurrentThreadId(), _ReturnAddress());
				printf_s("CfxSendTo: Data: %s Hex: %s\n", buf, string_to_hex(buf));
			}
		}
		else
		{
			char publicAddr[256];
			inet_ntop(AF_INET, &toIn->sin_addr.s_addr, publicAddr, sizeof(publicAddr));

			if (toIn->sin_addr.s_addr == 0xFFFFFFFF)
			{
				return len;
			}

			printf_s("CfxSendTo (to %s) port: %i, %i b, %p\n", publicAddr, htons(toIn->sin_port), len, _ReturnAddress());
		}

		//g_netLibrary->RoutePacket(buf, len, (uint16_t)((htonl(toIn->sin_addr.s_addr)) & 0xFFFF) ^ 0xFEED);

		//return len;
	}

	return sendto(s, buf, len, flags, to, tolen);
}

int __stdcall CfxRecv(SOCKET s, char * buf, int len, int flags)
{
#if _DEBUG
	printf_s("CfxRecv: Data: %s Hex: %s, Addr: %p\n", buf, string_to_hex(buf), _ReturnAddress());
#endif
	return recv(s, buf, len, flags);
}

int __stdcall CfxSend(SOCKET s, char * buf, int len, int flags)
{
#if _DEBUG
	printf_s("CfxSend: Data: %s Hex: %s, Addr: %p\n", buf, string_to_hex(buf), _ReturnAddress());
#endif
	return send(s, buf, len, flags);
}

#include "qr2.h"
static intptr_t(*g_origsend_heartbeat)(qr2_t qrec, int statechanged);
static intptr_t send_heartbeat(qr2_t qrec, int statechanged)
{
	return g_origsend_heartbeat(qrec, statechanged);
}

static int(*g_origmenuGUIHookThing)(int menuOption, bool result);
static int menuGUIHookThing(int menuOption, bool result) {
	__try
	{
		return g_origmenuGUIHookThing(menuOption, result);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

		printf_s("menuGUIHookThing excepted, caught and returned for menuOption %d with result %s at address %p.\n", menuOption, result > 0 ? "True" : "False", _ReturnAddress());
	}
}

static int(*g_origunkHookThing)(intptr_t a1);
static int unkHookThing(intptr_t a1)
{
	__try
	{
		return g_origunkHookThing(a1);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

		printf_s("unkHookThing excepted harder than a christmas tree at address %p.\n", _ReturnAddress());
	}
}

static int(*g_origunkHookThing2)();
static int unkHookThing2()
{
	__try
	{
		return g_origunkHookThing2();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

		printf_s("unkHookThing2 excepted harder than a christmas tree %p.\n", _ReturnAddress());
	}
}

static bool(*funcBuildQueryOrig)(int a1, int a2, int a3, struct sockaddr *to);
static bool funcBuildQueryCustom(int a1, int a2, int a3, struct sockaddr *to)
{
	if (a2 == 92)
		printf_s("a2 was 92, calling sendto!!!!\n");

	printf_s("funcBuildQueryCustom(%d, %d, %d, %d) invoked...\n", a1, a2, a3, to);
	return funcBuildQueryOrig(a1, a2, a3, to);
}

static SBError ServerListConnect(void *padding1, SBServerList *slist)
{
	struct   sockaddr_in masterSAddr;
	struct hostent *hent;
	char masterHostname[128];
	int masterIndex;


	masterIndex = StringHash(slist->queryforgamename, NUM_MASTER_SERVERS);
	if (SBOverrideMasterServer != NULL)
		strcpy(masterHostname, SBOverrideMasterServer);
	else //use the default format...
		sprintf(masterHostname, "%s.ms%d." GSI_DOMAIN_NAME, slist->queryforgamename, masterIndex);
	masterSAddr.sin_family = AF_INET;
	masterSAddr.sin_port = htons(28910);
	masterSAddr.sin_addr.s_addr = inet_addr(masterHostname);
	if (masterSAddr.sin_addr.s_addr == INADDR_NONE)
	{
		hent = gethostbyname(masterHostname);
		if (!hent)
			return sbe_dnserror;
		memcpy(&masterSAddr.sin_addr.s_addr, hent->h_addr_list[0], sizeof(masterSAddr.sin_addr.s_addr));
	}

	if (slist->slsocket == INVALID_SOCKET)
	{
		slist->slsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (slist->slsocket == INVALID_SOCKET)
			return sbe_socketerror;
	}
	if (connect(slist->slsocket, (struct sockaddr *) &masterSAddr, sizeof masterSAddr) != 0)
	{
		closesocket(slist->slsocket);
		slist->slsocket = INVALID_SOCKET;
		return sbe_connecterror;
	}

	//else we are connected
	return sbe_noerror;

}

#ifdef CONSOLE_ENABLED_THAT_CRASHES
static HookFunction hookFunction([]()
{
#if _DEBUG
	hook::vp::jump(CLOG_DEBUG_FUNC_ADDR, logFuncCustom); //CLog::Debug
	hook::vp::jump(CLOG_NORMAL_FUNC_ADDR, logFuncCustom); //CLog::Normal
	hook::vp::jump(CLOG_WARNING_FUNC_ADDR, logFuncCustom); //CLog::Warning
	hook::vp::jump(CLOG_ERROR_FUNC_ADDR, logFuncCustom); //CLog::Error
	hook::return_function_vp(NO_INTRO_SCREENS_FUNC_ADDR, 0); //no intro screens

	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
#endif
});
#endif

HMONITOR GetPrimaryMonitorHandle()
{
	const POINT ptZero = { 0, 0 };
	return MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
}

static LARGE_INTEGER dwPerfCount;
static char* savedPerfCount;

static void VC_QPCNEW() {
	dwPerfCount = *(LARGE_INTEGER*)savedPerfCount;
}

static DWORD VC_RETURNQPC() {
	DWORD result;
	LARGE_INTEGER v1;
	QueryPerformanceCounter(&v1);
	result = (v1.LowPart - v1.LowPart) & 0x7FFFFFFF;
	return result;
}

bool ReturnTrue() {
	return true;
}
static void(*ournewSehFunc)(intptr_t* a1);

#include "GS_Fifo.h"
int NTSLengthSB(char *buf, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		if (buf[i] == '\0') //found a full NTS
			return i + 1; //return the length including the null
	}
	return -1;
}

bool qr2_internal_is_master_only_key(const char * keyname)
{
	//COUNTRY_KEY = 31, REGION_KEY = 32
	if (strcmp(keyname, qr2_registered_key_list[31]) == 0 ||
		strcmp(keyname, qr2_registered_key_list[32]) == 0)
		return true;

	return false;
}

static void(*SBServerAddKeyValue)(SBServer server, const char *keyname, const char *value);
static void(*SBServerParseQR2FullKeysSingle)(SBServer server, char *data, int len);
static void(*QEStartQuery)(SBQueryEngine *engine, SBServer server);
static void(*ProcessIncomingReplies)(SBQueryEngine *engine, bool icmpSocket);
static void(*TimeoutOldQueries)(SBQueryEngine *engine);
static void(*QueueNextQueries)(SBQueryEngine *engine);
static void(*InitCryptKey)(SBServerList *slist, char *key, int keylen);
const char*(*SBRefStr)(SBServerList *slist, const char *str);
void(*GOADecrypt)(GOACryptState *state, unsigned char *bp, int len);
//static int(*IncomingListParseServer)(SBServerList *slist, char *buf, int len);
// FullKeys with split response support
//   Goes something like:
//       [qr2header]["splitnum"][num (byte)][keytype]
//            if keytype is server, read KV's until a NULL
//            otherwise read a keyname, then a number of values until NULL
#define QR2_SPLITPACKET_NUMSTRING "splitnum"
#define QR2_SPLITPACKET_MAX	      7      // -xxxxxxx
#define QR2_SPLITPACKET_FINAL     (1<<7) // x-------
void SBServerParseQR2FullKeysSplit(SBServer server, char *data, int len)
{
	int dlen;
	char *k;
	char *v;
	unsigned int packetNum = 0;
	bool isFinal = false;

	// make sure it's valid
	if (*data == '\0')
		return;

	// data should have "splitnum" followed by BINARY key
	dlen = NTSLengthSB(data, len);
	if (dlen < 0)
		return;
	k = data;
	data += dlen;
	len -= dlen;

	if (_strnicmp(QR2_SPLITPACKET_NUMSTRING, k, strlen(QR2_SPLITPACKET_NUMSTRING)) != 0)
		return;
	if (len < 1)
		return;

	packetNum = (unsigned int)((unsigned char)*data);
	data++;
	len--;

	// check final flag
	if ((packetNum & QR2_SPLITPACKET_FINAL) == QR2_SPLITPACKET_FINAL)
	{
		isFinal = true;
		packetNum ^= QR2_SPLITPACKET_FINAL;
	}

	// sanity check the packet num
	if (packetNum > QR2_SPLITPACKET_MAX)
		return;

	// update the received flags
	if (isFinal == true)
		// mark all bits higher than the final packet
		server->splitResponseBitmap |= (char)(0xFF << packetNum);
	else
		// mark only the bit for the received packet
		server->splitResponseBitmap |= (1 << packetNum);

	// any data in this packet? (could be a final tag)
	if (len < 1)
		return;

	// Parse the key sections (server/player/team)
	while (len > 0)
	{
		int keyType = 0;
		int nindex = 0;

		// Read the key type
		keyType = *data;
		data++;
		len--;

		if (keyType < 0 || keyType > 2)
		{
			printf_s("Split packet parse error, invalid key type! (%d)\n", keyType);
			return; // invalid key type!
		}

		// read keys until <null> section terminator
		while (*data)
		{
			// Read key name
			dlen = NTSLengthSB(data, len);
			if (dlen < 0)
				return;
			k = data;
			data += dlen;
			len -= dlen;

			if (keyType == 0)
			{
				// read the server key value
				dlen = NTSLengthSB(data, len);
				if (dlen < 0)
					return;
				v = data;
				data += dlen;
				len -= dlen;

				//add the value if its not a Query-From-Master-Only key
				if (!qr2_internal_is_master_only_key(k))
				{
					SBServerAddKeyValue(server, k, v);
				}
			}
			else
			{
				char tempkey[128];

				// Read first player/team number
				if (len < 1)
					return;

				nindex = *data;
				data++;
				len--;

				// read values until <null>
				while (*data)
				{
					// read the value
					dlen = NTSLengthSB(data, len);
					if (dlen < 0)
						return;
					v = data;
					data += dlen;
					len -= dlen;
					// append team or player index before adding
					sprintf(tempkey, "%s%d", k, nindex);
					SBServerAddKeyValue(server, tempkey, v);
					nindex++; // index increments from start
				}

				// skip the null (key terminator)
				if (len > 0)
				{
					data++;
					len--;
				}
			}
		}

		// skip the null (section terminator)
		if (len > 0)
		{
			if (*data != '\0')
			{
				printf_s("Split packet parse error, NULL byte expected!\n");
				return; // ERROR!
			}
			data++;
			len--;
		}
	}
}

BOOL __cdecl sub_442070(SOCKET a1)
{
	int v1; // eax@1
	struct timeval timeout; // [sp+0h] [bp-10Ch]@1
	fd_set readfds; // [sp+8h] [bp-104h]@1
	
	readfds.fd_array[0] = a1;
	readfds.fd_count = 1;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	v1 = select(64, &readfds, 0, 0, &timeout);
	bool selected = (v1 != -1 && v1);

	if (selected)
		logFuncCustom(NULL, "CanReceiveOnSocket: '%s', retAddr: %p\n", selected ? "Yes" : "No", _ReturnAddress());

	return selected;
}

static void SBQueryEngineThink(SBQueryEngine *engine)
{
	if(*(int*)(engine + 16) == 0)
		return;

	ProcessIncomingReplies(engine, false);
#ifdef SB_ICMP_SUPPORT
	ProcessIncomingReplies(engine, SBTrue);
#endif
	
	TimeoutOldQueries(engine);
	if (*(int*)(engine + 28) > 0)
		QueueNextQueries(engine);

	if (*(int*)(engine + 16) == 0) //we are now idle..
		(*(int(__cdecl **)(SBQueryEngine*, signed int, DWORD, DWORD))(engine + 64))(engine, qe_engineidle, NULL, *(DWORD*)(engine + 68));     // engine->ListCallback(engine, qe_engineidle, NULL, engine->instance);
		//engine->ListCallback(engine, qe_engineidle, NULL, engine->instance);
}

void __declspec(naked) SBQueryEngineThinkWrapper() {
	__asm {
		push esi
		call SBQueryEngineThink
		add esp, 4
		retn
	}
}

void orig_ParseSingleQR2Reply(SBQueryEngine *engine, SBServer server, char *data, int len) {
#if 1
	int i;
	int dlen;

	// 0x00 == qr2 query response, 0x09 == qr2 challenge response
	if (data[0] != 0x00 && data[0] != 0x09)
		return;

	//we could test the request key here for added security, or skip
	data += 5;
	len -= 5;
	if (server->state & STATE_PENDINGQUERYCHALLENGE)
	{
		server->state &= (unsigned char)~(STATE_PENDINGQUERYCHALLENGE);

		if (len > 0)
		{
			server->querychallenge = (gsi_u32)atoi(data);
			FIFORemove(&engine->querylist, server); // remove it
			QEStartQuery(engine, server); // readd it with a keys query
			engine->ListCallback(engine, qe_challengereceived, server, engine->instance);
			return;
		}
	}
	else if (server->state & STATE_PENDINGBASICQUERY)
	{
		//need to pick out the keys they selected
		for (i = 0; i < engine->numserverkeys; i++)
		{
			dlen = NTSLengthSB(data, len);
			if (dlen < 0)
				break;
			//add the value if its not a Query-From-Master-Only key
			if (!qr2_internal_is_master_only_key(qr2_registered_key_list[engine->serverkeys[i]]))
			{
				SBServerAddKeyValue(server, qr2_registered_key_list[engine->serverkeys[i]], data);
			}
			data += dlen;
			len -= dlen;
		}
		server->state |= STATE_BASICKEYS | STATE_VALIDPING;
	}
	else //need to parse out all the keys
	{
		// Is this a split packet format?
		if (*data && strncmp("splitnum", data, 8) == 0)
		{
			SBServerParseQR2FullKeysSplit(server, data, len);
			if (server->splitResponseBitmap != 0xFF)
				return;
			server->state |= STATE_FULLKEYS | STATE_BASICKEYS | STATE_VALIDPING;
		}
		else
		{
			// single packet
			SBServerParseQR2FullKeysSingle(server, data, len);
			server->state |= STATE_FULLKEYS | STATE_BASICKEYS | STATE_VALIDPING;
		}
	}
	server->state &= (unsigned char)~(STATE_PENDINGBASICQUERY | STATE_PENDINGFULLQUERY);
	server->updatetime = GetTickCount() - server->updatetime;
	FIFORemove(&engine->querylist, server);
	engine->ListCallback(engine, qe_updatesuccess, server, engine->instance);
#endif
}

static int ServerSizeForFlags(int flags)
{
	int size = 5; //all servers are at least 5 ..
	if (flags & PRIVATE_IP_FLAG)
		size += 4;
	if (flags & ICMP_IP_FLAG)
		size += 4;
	if (flags & NONSTANDARD_PORT_FLAG)
		size += 2;
	if (flags & NONSTANDARD_PRIVATE_PORT_FLAG)
		size += 2;
	return size;

}

//checks to see if all the keys for the given servers are there
static int AllKeysPresent(SBServerList *slist, char *buf, int len)
{
	int numkeys;
	int i;
	int strindex;
	numkeys = ArrayLength(slist->keylist);
	for (i = 0; i < numkeys; i++)
	{
		switch (((KeyInfo *)ArrayNth(slist->keylist, i))->keyType)
		{
		case KEYTYPE_BYTE:
			buf++;
			len--;
			break;
		case KEYTYPE_SHORT:
			buf += 2;
			len -= 2;
			break;
		case KEYTYPE_STRING:
			if (len < 1)
				return 0; //not enough
			strindex = (unsigned char)(buf[0]);
			buf++;
			len--;
			if (strindex == 0xFF) //a NTS string
			{
				strindex = NTSLengthSB(buf, len);
				if (strindex == -1) //not all there..
					return 0;
				buf += strindex;
				len -= strindex;
			} //else it's a popular string - just the index is present
			break;
		default:
			GS_FAIL();
			return 0; //error - unknown key type
		}
		if (len < 0)
			return 0; //not enough..
	}
	return 1;
}


//parse only the IP/port from the server buffer
static void ParseServerIPPort(SBServerList *slist, char *buf, int len, goa_uint32 *ip, unsigned short *port)
{
	unsigned char flags;
	if (len < 5)
		return; //invalid buffer
	flags = (unsigned char)buf[0];
	buf++;
	len--;
	memcpy(ip, buf, 4);

	buf += 4;
	len -= 4;

	if (flags & NONSTANDARD_PORT_FLAG)
	{
		if (len < 2)
			return; //invalid buffer
		memcpy(port, buf, 2);
	}
	else
		*port = slist->defaultport;
}

static int FullRulesPresent(char *buf, int len)
{
	int i;
	while (len > 0 && *buf)
	{
		i = NTSLengthSB(buf, len);
		if (i < 0)
			return 0;  //no full key
		buf += i;
		len -= i;
		i = NTSLengthSB(buf, len);
		if (i < 0)
			return 0; //no full value
		buf += i;
		len -= i;
	}
	if (len == 0)
		return 0; //not even enough space for the null term
	if (*buf == 0)
		return 1; //all there
	return 0;
}

int SBIsNullServer(SBServer server)
{
	return (server == SBNullServer) ? 1 : 0;
}

static int __cdecl IncomingListParseServer(SBServerList *slist, char *buf, int len)
{
	int i;
	goa_uint32 ip;
	unsigned short port;
	SBServer server;
	unsigned char flags;
	//fields depends on the flags..
	if (len < 1)
		return 0;
	flags = (unsigned char)(buf[0]);
	i = ServerSizeForFlags(flags);
	if (len < i)
		return 0;
	if (flags & HAS_KEYS_FLAG)
	{
		if (!AllKeysPresent(slist, buf + i, len - i))
			return 0;
	}
	if (flags & HAS_FULL_RULES_FLAG)
	{
		if (!FullRulesPresent(buf + i, len - i))
			return 0;
	}
	//else we have a whole server!
	//see if it's the "last" server (0xffffffff)
	if (memcmp(buf + 1, LAST_SERVER_MARKER, SERVER_MARKER_LEN) == 0)
		return -1;
	ParseServerIPPort(slist, buf, len, &ip, &port);

#if 0
	server = SBAllocServer_Fake(slist, ip, port);
	if (SBIsNullServer(server))
		return -2;
	i = ParseServer(slist, server, buf, len, 1);
	SBServerListAppendServer(slist, server);
#endif
	return i;
}

static SBError __cdecl ProcessMainListData(void* ffs, SBServerList *slist)
{
	int bytesRead;
	int keyoffset;
	int keylen;
	char *inbuf = slist->inbuffer;
	int inlen = slist->inbufferlen;
	int pstate = *(int *)(slist + 0x5B8);
	void* slistCopy = &slist;

	switch (/*slist->pstate*/pstate)
	{
	case pi_cryptheader:
		if (inlen < 1)
			break;
		// We get the length of data up to the key
		// We add 2 because we're ignoring the lengths in two of the bytes
		bytesRead = (((unsigned char)(inbuf[0])) ^ 0xEC) + 2;
		if (inlen < bytesRead)
			break; //not there yet

		keyoffset = bytesRead;
		keylen = ((unsigned char)(inbuf[bytesRead - 1])) ^ 0xEA;
		bytesRead += keylen;
		if (inlen < bytesRead)
			break; //not there yet
				   //otherwise we have the whole crypt header and can init our crypt key
		
		InitCryptKey((SBServerList*)&slistCopy, inbuf + keyoffset, keylen);
		slist->pstate = pi_fixedheader;

		// the first byte of the "random" data is actually
		// the game options flag
		memcpy(&slist->backendgameflags, &inbuf[1], 2);
		slist->backendgameflags = ntohs(slist->backendgameflags);

		inbuf += bytesRead;
		inlen -= bytesRead;
		//decrypt any remaining data!
		GOADecrypt(&(slist->cryptkey), (unsigned char *)inbuf, inlen);
		//and fall through
	case pi_fixedheader:
		if (inlen < FIXED_HEADER_LEN)
			break;
		memcpy(&slist->mypublicip, inbuf, 4);
		slist->ListCallback(slist, slc_publicipdetermined, SBNullServer, slist->instance);
		memcpy(&slist->defaultport, inbuf + 4, 2);
		if (slist->defaultport == 0xFFFF) //there was an error-  grab the error string if present.. 
		{
			if (NTSLengthSB(inbuf + FIXED_HEADER_LEN, inlen - FIXED_HEADER_LEN) == -1) //not all there
				break;
			//egh
			//SBSetLastListErrorPtr(slist, inbuf + FIXED_HEADER_LEN);
			//make the error callback
			slist->ListCallback(slist, slc_queryerror, SBNullServer, slist->instance);
			if (slist->inbuffer == NULL)
				break;
		}
		inbuf += FIXED_HEADER_LEN;
		inlen -= FIXED_HEADER_LEN;
		if ((slist->queryoptions & NO_SERVER_LIST) || slist->defaultport == 0xFFFF)
		{
			slist->pstate = pi_finished;
			slist->state = sl_connected;
			break; //no more data expected (if we didn't request any more, or the server doesn't know about this game)
		}
		slist->pstate = pi_keylist;
		slist->expectedelements = -1;

		//and fall through
	case pi_keylist:
		if (slist->expectedelements == -1) //we haven't read the initial count of keys yet..
		{
			if (inlen < 1)
				break;
			slist->expectedelements = (unsigned char)(inbuf[0]);
			slist->keylist = ArrayNew(sizeof(KeyInfo), slist->expectedelements, NULL);
			if (slist->keylist == NULL) //error
				return sbe_allocerror;
			inbuf++;
			inlen--;
		}
		//try to read elements, up to the expected amount...
		while (slist->expectedelements > ArrayLength(slist->keylist))
		{
			KeyInfo ki;
			int keylen;
			if (inlen < 2)
				break; //can't possibly be a full key (keytype + string)
			keylen = NTSLengthSB(inbuf + 1, inlen - 1);
			if (keylen == -1)
				break; //no full NTS string there
			ki.keyType = (unsigned char)(inbuf[0]);
			ki.keyName = SBRefStr(slist, inbuf + 1);
			ArrayAppend(slist->keylist, &ki);
			inbuf += keylen + 1;
			inlen -= keylen + 1;
		}
		if (slist->expectedelements > ArrayLength(slist->keylist))
			break; //didn't read them all...
				   //else we have read all the keys, fall through..
		slist->pstate = pi_uniquevaluelist;
		slist->expectedelements = -1;
	case pi_uniquevaluelist:
		if (slist->expectedelements == -1) //we haven't read the # of unique values yet..
		{
			if (inlen < 1)
				break;
			slist->expectedelements = (unsigned char)(inbuf[0]);
			slist->numpopularvalues = 0;
			inbuf++;
			inlen--;
		}
		while (slist->expectedelements > slist->numpopularvalues)
		{
			int keylen = NTSLengthSB(inbuf, inlen);
			if (keylen == -1)
				break; //no full NTS string
			slist->popularvalues[slist->numpopularvalues++] = SBRefStr(slist, inbuf);
			inbuf += keylen;
			inlen -= keylen;
		}
		if (slist->expectedelements > slist->numpopularvalues)
			break; //didn't read all the popular values
				   //else we've got them all - move on to servers!
		slist->pstate = pi_servers;
	case pi_servers:
		if (inlen < 5) //not enough for a full servers
			break;
		do
		{
			bytesRead = IncomingListParseServer(slist, inbuf, inlen);
			if (bytesRead == -2)
				return sbe_allocerror;
			else if (bytesRead == -1) //that was the last server!
			{
				inlen -= 5;
				inbuf += 5;
				slist->pstate = pi_finished;
				slist->state = sl_connected;
				slist->ListCallback(slist, slc_initiallistcomplete, SBNullServer, slist->instance);
				break;
			}
			inbuf += bytesRead;
			inlen -= bytesRead;
			if (slist->inbuffer == NULL)
				bytesRead = 0; //break out - they disconnected
		} while (bytesRead != 0);
		break;
	default:
		break;
	}
	GS_ASSERT(inlen >= 0);
	if (slist->inbuffer == NULL)
		return sbe_noerror; //don't keep processing - they disconnected
	if (inlen != 0) //need to shift it over..
	{
		memmove(slist->inbuffer, inbuf, (size_t)inlen);
	}
	slist->inbufferlen = inlen;
	//we could clear the key list here if we wanted

	return sbe_noerror;
}

int __declspec(naked) ProcessMainListDataWrapper() {
	__asm {
		push esi
		call ProcessMainListData
		add esp, 4
		retn
	}
}

void wrapSEH(intptr_t* a1) {
	__try
	{
		ournewSehFunc(a1);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		printf_s("wrapSEH excepted, caught and returned.\n");
	}
}

void customCallbackPatch(int reason) {
	logFuncCustom(NULL, "SBCallback called with reason: %d from %p!\n", reason, _ReturnAddress());
	return;
}

void(*g_origSBCallback)(intptr_t* serverBrowser, int reason, SBServer server, void *instance);
DWORD jmpAddrExit = SB_CALLBACK_JMPADDR_EXIT;
DWORD jmpAddrContinue = SB_CALLBACK_JMPADDR_CONT;
int reason;
void __declspec(naked) SBCallbackPatch() {
#ifndef COMPILING_2005
	__asm {
		push eax
		mov eax, dword ptr ds:[esp+0xBC]
		mov eax, dword ptr ds:[eax]
		cmp eax, 1
		jg label
			jmp label2
		label:
			push eax
			call customCallbackPatch
			add esp, 4
			jmp jmpAddrExit
		label2:
			push eax
			call customCallbackPatch
			add esp, 4
			pop eax
			jmp jmpAddrContinue
	}
#else
	//2005
	__asm {
		push esi
		mov esi, dword ptr ds:[esp + 0x154]
		cmp esi, 1
		pop esi
		jg label
			jmp jmpAddrContinue
		label:
			jmp jmpAddrExit
	}
#endif
}
//#define EXPERIMENTAL_HOOKS

void exitFunc(int code) {
	printf("Exited at: %p\n", _ReturnAddress());
	return;
}
int(*orig_ProcessIncomingData_Hook)(SBServerList slist);

void ourProtoNetFunc(SBServerList* slist) {
	char* val = *(char**)(slist + 16);
	//printf("%p\n", val);
	return;
}

void _declspec(naked) ProcessIncomingData_Hook()
{
	_asm
	{
		push eax
		push ecx
		call ourProtoNetFunc
		pop ecx
		pop eax

		jmp orig_ProcessIncomingData_Hook
	}
}

#define MASTER_URL "master.openspy.org"
hostent* __stdcall CfxHostname(char* hostname) {
	char modifiedHostName[256];
	strcpy(modifiedHostName, strstr(hostname, "gamespy") ? MASTER_URL : hostname);
	logFuncCustom(NULL, "CfxHostname called with hostname: %s from %p, patched to %s!\n", hostname, _ReturnAddress(), modifiedHostName);
	return gethostbyname(modifiedHostName);
}

//https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
template< typename T >
std::string int_to_hex(T i)
{
	std::stringstream stream;
	stream << "0x"
		<< std::setfill('0') << std::setw(sizeof(T) * 2)
		<< std::hex << i;
	return stream.str();
}

void checkGameVersion() {
	auto location = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 50 E8 ? ? ? ? 8B 0D").count(1).get(0).get<intptr_t>(0);
	intptr_t version = (*(intptr_t*)location);
	version = (intptr_t)(version / 0x100); //Remove the last two digits on the address
	version = *(intptr_t*)version; //Finally, access the address

	std::string verOut = int_to_hex(version);

	//https://stackoverflow.com/questions/3790613/how-to-convert-a-string-of-hex-values-to-a-string
	int len = verOut.length();
	std::string newString;
	for (int i = 0; i< len; i += 2)
	{
		std::string byte = verOut.substr(i, 2);
		char chr = (char)(int)strtol(byte.c_str(), NULL, 16);
		newString.push_back(chr);
	}

	std::reverse(newString.begin(), newString.end());
	if (strcmp(newString.c_str(), GAME_VERSION_NEEDED)) {
		char errorMsg[128];
		snprintf(errorMsg, sizeof(errorMsg), "You currently have Deer Hunter Patch v%s installed but to run this patch you require to have Deer Hunter v%s.", newString.c_str(), GAME_VERSION_NEEDED);
		MessageBox(0, errorMsg, "Game Version Error", MB_ICONERROR);
		exit(0);
	}
}
bool VCPatcher::Init()
{
	//53 8B 5C 24 08 2B 59 0C
	//53 8B 5C 24 08 2B 59 0C
	char* loc;
#ifdef EXPERIMENTAL_HOOKS
	char* location = hook::pattern("E8 ? ? ? ? 85 C0 0F 85 ? ? ? ? 8B B4 24").count(1).get(0).get<char>(0);
	hook::vp::call(location, ServerListConnect);
#ifdef NEW_HOOKS	
	loc = (char*)0x409D00;
	MH_CreateHook(loc, menuGUIHookThing, (void**)&g_origmenuGUIHookThing);
#endif
	loc = (char*)0x441BF0;
	MH_CreateHook(loc, unkHookThing, (void**)&g_origunkHookThing);

	loc = (char*)0x4090A0; //StartServer (excepts at aspen.dll)
	MH_CreateHook(loc, unkHookThing2, (void**)&g_origunkHookThing2);

	loc = (char*)0x443850; //sendheartbeat
	MH_CreateHook(loc, send_heartbeat, (void**)&g_origsend_heartbeat);
	
	MH_EnableHook(MH_ALL_HOOKS);
#endif

	checkGameVersion();
//#ifndef COMPILING_2005
	hook::nopVP(NOP_VP_CALLBACK_LOGIC, 6); //nop if statement at sbcallback to show the servers
	hook::vp::jump(NOP_VP_CALLBACK_LOGIC, SBCallbackPatch); //ServerBrowser callback logic re-impl'd...
//#endif
	loc = (char*)FUNC_BUILD_QUERY_FUNC;
	hook::set_call(&funcBuildQueryOrig, loc);
	hook::vp::call(loc, funcBuildQueryCustom);

	hook::iat("WSOCK32.dll", CfxSend, 19);
	hook::iat("WSOCK32.dll", CfxSendTo, 20);
	hook::iat("WSOCK32.dll", CfxRecv, 16);
	hook::iat("WSOCK32.dll", CfxRecvFrom, 17);
	hook::iat("WSOCK32.dll", CfxBind, 2);
	hook::iat("WSOCK32.dll", CfxHostname, 52);
#ifdef COMPILING_2005
	//hook::vp::call(0x4FEC1B, exitFunc);
	hook::putVP<uint8_t>(0x594578, 0xEB); //Skip the former error 183 at startup which doesn't allow you to start multiple instances of the game for example when hosting a dedicated server and running the game.
	//hook::nopVP(0x558818, 2); //nop this thing at processincomingreplies
	//hook::nopVP(0x4FFFFE, 6);
	hook::set_call(&orig_ProcessIncomingData_Hook, 0x592C6E);
	hook::vp::call(0x592C6E, ProcessIncomingData_Hook);
#endif

#ifdef _DEBUG
	//loc = (char*)0x441070;
	//SBCallback(int serverBrowser, int reason, int server, int instance)
	//MH_CreateHook(loc, SBCallbackPatch, (void**)&g_origSBCallback);

	//hook::vp::jump(0x446FD0, ProcessMainListData);
	//hook::vp::jump(0x446ED0, IncomingListParseServer);
#ifndef COMPILING_2005 //04 specific for now
	hook::vp::jump(PARSE_SINGLE_QR2_REPLY, orig_ParseSingleQR2Reply);
	hook::vp::jump(SBQueryEngineThinkWrapperAddr, SBQueryEngineThinkWrapper);
	hook::vp::jump(sub_442070_Addr, sub_442070);

	hook::set_call(&SBServerAddKeyValue, SBServerAddKeyValueAddr);
	hook::set_call(&SBServerParseQR2FullKeysSingle, SBServerParseQR2FullKeysSingleAddr);
	hook::set_call(&QEStartQuery, QEStartQueryAddr);
	hook::set_call(&ProcessIncomingReplies, ProcessIncomingRepliesAddr);
	hook::set_call(&TimeoutOldQueries, TimeoutOldQueriesAddr);
	hook::set_call(&QueueNextQueries, QueueNextQueriesAddr);
	hook::set_call(&SBRefStr, SBRefStrAddr);
	hook::set_call(&GOADecrypt, GOADecryptAddr);
	//hook::set_call(&IncomingListParseServer, IncomingListParseServerAddr);
	hook::set_call(&InitCryptKey, InitCryptKeyAddr);
#endif
		//static void QEStartQuery(SBQueryEngine *engine, SBServer server)
#endif
	/*
	HMODULE hAspen = GetModuleHandle("Aspen.dll");
	if (hAspen)
	{
		auto pattern = hook::module_pattern(hAspen, "84 34 12 10 F9 00 00 00 0C F9 00 80").count(1);
		if (pattern.size() == 1) {
			char *location = pattern.get(0).get<char>(0x82);
			hook::vp::call(location, CfxRecvFrom);
		}
	}
	*/
	//g_oldSelect = hook::iat("ws2_32.dll", CfxSelect, 18);
	//hook::iat("ws2_32.dll", CfxGetSockName, 6);

	//hook::set_call(&ournewSehFunc, 0x4458D0);
	//hook::vp::call(0x4458D0, wrapSEH);
	return true;
}

static struct MhInit
{
	MhInit()
	{
		MH_Initialize();
	}
} mhInit;

bool VCPatcher::PatchResolution(D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	pPresentationParameters->Windowed = true;
	pPresentationParameters->Flags = 0;
	pPresentationParameters->FullScreen_RefreshRateInHz = 0;
	pPresentationParameters->FullScreen_PresentationInterval = 0;

	pPresentationParameters->BackBufferWidth = 1280;
	pPresentationParameters->BackBufferHeight = 720;

	SetWindowPos(pPresentationParameters->hDeviceWindow, HWND_NOTOPMOST, 0, 0, pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight, SWP_SHOWWINDOW);
	SetWindowLong(pPresentationParameters->hDeviceWindow, GWL_STYLE, WS_POPUP | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE);
	return true;
}