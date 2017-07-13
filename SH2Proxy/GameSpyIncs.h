//Error codes that can be returned from functions
#ifndef GAMESPY_INCS_H
#define GAMESPY_INCS_H
#include "darray.h"
#include <winsock.h>
typedef enum
{
	sbe_noerror,  //no error has occured
	sbe_socketerror, //a socket function has returned an unexpected error
	sbe_dnserror,  //DNS lookup of master address failed
	sbe_connecterror,  //connection to master server failed
	sbe_dataerror, //invalid data was returned from master server
	sbe_allocerror, //memory allocation failed
	sbe_paramerror,	//an invalid parameter was passed to a function
	sbe_duplicateupdateerror //server update requested on a server that was already being updated
} SBError;


typedef enum {
	pi_cryptheader,
	pi_fixedheader,
	pi_keylist,
	pi_uniquevaluelist,
	pi_servers,
	pi_finished
} SBListParseState;


typedef enum { sl_lanbrowse, sl_disconnected, sl_connected, sl_mainlist } SBServerListState;
typedef void(*ArrayElementFreeFn)(void *elem);

// STRUCTURES
struct DArrayImplementation
{
	int count, capacity;
	int elemsize;
	int growby;
	ArrayElementFreeFn elemfreefn;
	void *list; //array of elements
};

// Define GameSpy types

// common base type defines, please refer to ranges below when porting
typedef char              gsi_i8;
typedef unsigned char     gsi_u8;
typedef short             gsi_i16;
typedef unsigned short    gsi_u16;
typedef int               gsi_i32;
typedef unsigned int      gsi_u32;
typedef unsigned int      gsi_time;  // must be 32 bits

									 // Deprecated
typedef gsi_i32           goa_int32;  // 2003.Oct.04 - typename deprecated
typedef gsi_u32           goa_uint32;

typedef struct _SBServerList SBServerList;
typedef struct _SBServerList *SBServerListPtr;
typedef struct _SBQueryEngine *SBQueryEnginePtr;

//callback types for server lists
typedef enum { slc_serveradded, slc_serverupdated, slc_serverdeleted, slc_initiallistcomplete, slc_disconnected, slc_queryerror, slc_publicipdetermined, slc_serverchallengereceived } SBListCallbackReason;
//callback types for query engine
typedef enum { qe_updatesuccess, qe_updatefailed, qe_engineidle, qe_challengereceived } SBQueryEngineCallbackReason;

//callback function prototypes
typedef void(*SBListCallBackFn)(SBServerListPtr serverlist, SBListCallbackReason reason, void* server, void *instance);
typedef void(*SBEngineCallbackFn)(SBQueryEnginePtr engine, SBQueryEngineCallbackReason reason, void* server, void *instance);
typedef void(*SBMaploopCallbackFn)(SBServerListPtr serverlist, void* server, time_t mapChangeTime, int numMaps, char *mapList[], void *instance);
typedef void(*SBPlayerSearchCallbackFn)(SBServerListPtr serverlist, char *nick, goa_uint32 serverIP, unsigned short serverPort, time_t lastSeenTime, char *gamename, void *instance);

typedef struct _GOACryptState
{
	unsigned char cards[256];       // A permutation of 0-255.
	unsigned char rotor;            // Index that rotates smoothly
	unsigned char ratchet;                    // Index that moves erratically
	unsigned char avalanche;                  // Index heavily data dependent
	unsigned char last_plain;                 // Last plain text byte
	unsigned char last_cipher;                // Last cipher text byte
} GOACryptState;

//how long to make the outgoing challenge
#define LIST_CHALLENGE_LEN 8
//max number of values in a popular value list
#define MAX_POPULAR_VALUES 255
//number of master servers
#define NUM_MASTER_SERVERS 20

#define GSI_DOMAIN_NAME "gamespy.com"
//max number of keys for the basic key list
#define MAX_QUERY_KEYS 40

//states for SBServer->state 
#define STATE_BASICKEYS			(1 << 0)
#define STATE_FULLKEYS			(1 << 1)
#define STATE_PENDINGBASICQUERY	(1 << 2)
#define STATE_PENDINGFULLQUERY	(1 << 3)
#define STATE_QUERYFAILED		(1 << 4)
#define STATE_PENDINGICMPQUERY	(1 << 5)
#define STATE_VALIDPING	        (1 << 6)
#define STATE_PENDINGQUERYCHALLENGE (1 << 7)


//game server flags
#define UNSOLICITED_UDP_FLAG	1
#define PRIVATE_IP_FLAG			2
#define CONNECT_NEGOTIATE_FLAG	4
#define ICMP_IP_FLAG			8
#define NONSTANDARD_PORT_FLAG	16
#define NONSTANDARD_PRIVATE_PORT_FLAG	32
#define HAS_KEYS_FLAG					64
#define HAS_FULL_RULES_FLAG				128
#define LAST_SERVER_MARKER "\xFF\xFF\xFF\xFF"
#define SERVER_MARKER_LEN 4

//key types for the key type list
#define KEYTYPE_STRING	0
#define KEYTYPE_BYTE	1
#define KEYTYPE_SHORT	2

//SBServer is an abstract data type representing a single server.
#ifndef SBServer 
typedef struct _SBServer *SBServer;
#endif	

struct _SBServerList
{
	SBServerListState state;
	DArray servers;
	DArray keylist;
	char queryforgamename[36];
	char queryfromgamename[36];
	char queryfromkey[32];
	//char mychallenge[LIST_CHALLENGE_LEN];
	char *inbuffer;
	int inbufferlen;
	const char *popularvalues[MAX_POPULAR_VALUES];
	int numpopularvalues;
	int expectedelements;

	SBListCallBackFn ListCallback;
	SBMaploopCallbackFn MaploopCallback;
	SBPlayerSearchCallbackFn PlayerSearchCallback;
	void *instance;

	BYTE currsortinfo[259];
	BYTE prevsortinfo[259];

	bool sortascending;
	goa_uint32 mypublicip;
	goa_uint32 srcip;
	unsigned short defaultport;

	char *lasterror;
#ifdef GSI_UNICODE
	unsigned short *lasterror_W;
#endif

	SOCKET slsocket;
	gsi_time lanstarttime;
	int fromgamever;
	GOACryptState cryptkey;
	int queryoptions;
	SBListParseState pstate;
	gsi_u16 backendgameflags;

	const char* mLanAdapterOverride;

	SBServer* deadlist;
};
#define AVAILABLE_BUFFER_LEN(a) (MAX_DATA_SIZE - (a)->len)

#include "hashtable.h"

struct _SBServer
{
	goa_uint32 publicip;
	unsigned short publicport;
	goa_uint32 privateip;
	unsigned short privateport;
	goa_uint32 icmpip;
	unsigned char state;
	unsigned char flags;
	HashTable keyvals;
	gsi_time updatetime;
	gsi_u32 querychallenge;
	struct _SBServer *next;
	gsi_u8 splitResponseBitmap;
};

typedef struct _SBServerFIFO
{
	SBServer first;
	SBServer last;
	int count;
} SBServerFIFO;

typedef struct _SBQueryEngine
{
	int queryversion;
	int maxupdates;
	SBServerFIFO querylist;
	SBServerFIFO pendinglist;
	SOCKET querysock;
#if !defined(SN_SYSTEMS)
	SOCKET icmpsock;
#endif
	goa_uint32 mypublicip;
	unsigned char serverkeys[MAX_QUERY_KEYS];
	int numserverkeys;
	SBEngineCallbackFn ListCallback;
	void *instance;
} SBQueryEngine;

extern SBServer SBNullServer;

//key information structure
typedef struct _KeyInfo
{
	const char *keyName;
	int keyType;
} KeyInfo;

#define NO_SERVER_LIST			2
#define FIXED_HEADER_LEN		6
#endif