#ifndef GAME_OFFSETS_H
#define GAME_OFFSETS_H
#ifdef COMPILING_2005
//2005 offsets
#define CLOG_DEBUG_FUNC_ADDR			0x41B97E
#define CLOG_NORMAL_FUNC_ADDR			0x41B948
#define CLOG_WARNING_FUNC_ADDR			0x41B9C6
#define CLOG_ERROR_FUNC_ADDR			0x41B978
#define NO_INTRO_SCREENS_FUNC_ADDR		0x591E30

#define SB_CALLBACK_JMPADDR_EXIT		0x5A8496
#define SB_CALLBACK_JMPADDR_CONT		0x5A82D2

#define NOP_VP_CALLBACK_LOGIC			0x5A82CC
#define FUNC_BUILD_QUERY_FUNC			0x55C511

#if 0
//Not done from here
#define PARSE_SINGLE_QR2_REPLY			0x4448E8
#define SBQueryEngineThinkWrapperAddr	0x4449D0
#define sub_442070_Addr					0x442070			

#define SBServerAddKeyValueAddr			0x44471E
#define SBServerParseQR2FullKeysSingleAddr 0x444751
#define QEStartQueryAddr				0x4449AF
#define ProcessIncomingRepliesAddr		0x4449DC
#define TimeoutOldQueriesAddr			0x4449E1
#define QueueNextQueriesAddr			0x4449EF
#define SBRefStrAddr					0x444B3A
#define GOADecryptAddr					0x44704D
//#define IncomingListParseServerAddr 0x4472AB
#define InitCryptKeyAddr				0x44702D
#endif
#else
//2004 offsets
#define CLOG_DEBUG_FUNC_ADDR			0x4E7F4A
#define CLOG_NORMAL_FUNC_ADDR			0x4E7F5C
#define CLOG_WARNING_FUNC_ADDR			0x4E7FAA
#define CLOG_ERROR_FUNC_ADDR			0x4E7F56
#define NO_INTRO_SCREENS_FUNC_ADDR		0x4086F0

#define SB_CALLBACK_JMPADDR_EXIT		0x441208
#define SB_CALLBACK_JMPADDR_CONT		0x4410AF

#define NOP_VP_CALLBACK_LOGIC			0x4410A9
#define FUNC_BUILD_QUERY_FUNC			0x4437FA

#define PARSE_SINGLE_QR2_REPLY			0x4448E8
#define SBQueryEngineThinkWrapperAddr	0x4449D0
#define sub_442070_Addr					0x442070			

#define SBServerAddKeyValueAddr			0x44471E
#define SBServerParseQR2FullKeysSingleAddr 0x444751
#define QEStartQueryAddr				0x4449AF
#define ProcessIncomingRepliesAddr		0x4449DC
#define TimeoutOldQueriesAddr			0x4449E1
#define QueueNextQueriesAddr			0x4449EF
#define SBRefStrAddr					0x444B3A
#define GOADecryptAddr					0x44704D
//#define IncomingListParseServerAddr 0x4472AB
#define InitCryptKeyAddr				0x44702D
#endif
#endif