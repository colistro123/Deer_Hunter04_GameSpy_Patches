#ifndef QR2_H
#define QR2_H
#include "GameSpyIncs.h"
typedef unsigned char uchar;

#define QR2_IPVERIFY_ARRAY_SIZE     200   // allowed outstanding queries in those 4 seconds
#define REQUEST_KEY_LEN 4
#define RECENT_CLIENT_MESSAGES_TO_TRACK 10
#define MAX_DATA_SIZE 1400
#define MAX_REGISTERED_KEYS 254

//////////////////////////////////////////////////////////////
// qr2_error_t
// Summary
//		Constants returned from qr2_init and the error callback to signal an
//		 error condition.
typedef enum
{
	e_qrnoerror,			// No error occurred
	e_qrwsockerror,			// A standard socket call failed, e.g. exhausted resources
	e_qrbinderror,			// The SDK was unable to find an available port to bind on
	e_qrdnserror,			// A DNS lookup (for the master server) failed
	e_qrconnerror,			// The server is behind a NAT and does not support negotiation
	e_qrnochallengeerror,	// No challenge was received from the master. The common reasons for this error are: <br>
							//					1. Not enabling the NatNegotiate flag in the peerSetTitle or qr2_init calls - this should be  
							//					   PEERTrue (or 1 for QR2) for games that support NATs. Otherwise the master server will assume  
							//					   this server is not behind a NAT and can directly connect to it <br> 
							//					2. Calling qr2_buffer_add more than once on a particular key value <br>
							//					3. Firewall or NAT configuration is blocking incoming traffic. You may need to open ports to allow
							//					   communication (see related) <br> 
							//					4. Using the same port for socket communications - shared socket implementations with qr2/peer 
							//					   together <br> 
							//					5. The heartbeat packet has exceeded the max buffer size of 1400 bytes. Try abbreviating some of 
							//					   the custom keys to fit within the 1400 byte buffer. The reason for this restriction is to 
							//					   support as many routers as possible, as UDP packets beyond this data range are more inclined to
							//					   be dropped. <br> 
							//					6. "numplayers" or "maxplayers" being set to negative values <br> 
							//					7. Having 2 network adapters connected to an internal and external network, and the internal one 
							//					   is set as primary <br>

							qr2_error_t_count
} qr2_error_t;

typedef enum
{
	key_server,		// General information about the game in progress.
	key_player,		// Information about a specific player.
	key_team,		// Information about a specific team.
	key_type_count
} qr2_key_type;

struct qr2_ipverify_info_s
{
	struct sockaddr_in addr;      // addr = 0 when not in use
	gsi_u32            challenge;
	gsi_time           createtime;
};

struct qr2_implementation_s
{
	SOCKET hbsock;
	char gamename[64];
	char secret_key[64];
	char instance_key[REQUEST_KEY_LEN];
	typedef void server_key_callback;
	typedef void player_key_callback;
	typedef void team_key_callback;
	typedef void key_list_callback;
	typedef void playerteam_count_callback;
	typedef void adderror_callback;
	typedef void nn_callback;
	typedef void cm_callback;
	typedef void pa_callback;
	typedef void cc_callback;
	typedef void hr_callback;
	gsi_time lastheartbeat;
	gsi_time lastka;
	int userstatechangerequested;
	int listed_state;
	int ispublic;
	int qport;
	int read_socket;
	int nat_negotiate;
	struct sockaddr_in hbaddr;
	typedef void cdkeyprocess;
	int client_message_keys[RECENT_CLIENT_MESSAGES_TO_TRACK];
	int cur_message_key;
	unsigned int publicip;
	unsigned short publicport;
	void *udata;

	gsi_u8 backendoptions; // received from server inside challenge packet 
	struct qr2_ipverify_info_s ipverify[QR2_IPVERIFY_ARRAY_SIZE];
};

struct qr2_buffer_s
{
	char buffer[MAX_DATA_SIZE];
	int len;
};

struct qr2_keybuffer_s
{
	char keys[MAX_REGISTERED_KEYS];
	int numkeys;
};

typedef struct qr2_implementation_s *qr2_t;
typedef struct qr2_buffer_s *qr2_buffer_t;
typedef struct qr2_keybuffer_s *qr2_keybuffer_t;

static void qr_add_packet_header(qr2_buffer_t buf, char ptype, char *reqkey);
bool qr2_buffer_addA(qr2_buffer_t outbuf, const char *value);
bool qr2_buffer_add_int(qr2_buffer_t outbuf, int value);
#if 0
static void qr_build_query_reply(qr2_t qrec, qr2_buffer_t buf, int serverkeycount, uchar *serverkeys, int playerkeycount, uchar *playerkeys, int teamkeycount, uchar *teamkeys);
#endif

extern const char *qr2_registered_key_list[MAX_REGISTERED_KEYS];
#endif