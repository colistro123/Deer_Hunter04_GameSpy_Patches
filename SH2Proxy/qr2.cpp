#include "stdafx.h"
#include "gsAssert.h"
#include "qr2.h"
#include <stdio.h>
#include <winsock.h>

static void qr_add_packet_header(qr2_buffer_t buf, char ptype, char *reqkey)
{
	buf->buffer[0] = ptype;
	memcpy(buf->buffer + 1, reqkey, REQUEST_KEY_LEN);
	buf->len = REQUEST_KEY_LEN + 1;
}

bool qr2_keybuffer_add(qr2_keybuffer_t keybuffer, int keyid)
{
	// these are codetime not runtime errors, changed to assert
	if (keybuffer->numkeys >= MAX_REGISTERED_KEYS)
		return false;
	if (keyid < 1 || keyid > MAX_REGISTERED_KEYS)
		return false;

	keybuffer->keys[keybuffer->numkeys++] = (char)keyid;
	return true;
}

bool qr2_buffer_add_int(qr2_buffer_t outbuf, int value)
{
	char temp[20];
	sprintf(temp, "%d", value);
	return qr2_buffer_addA(outbuf, temp);
}

bool qr2_buffer_addA(qr2_buffer_t outbuf, const char *value)
{
	GS_ASSERT(outbuf);
	GS_ASSERT(value);
	{
		int copylen;
		copylen = (int)strlen(value) + 1;
		if (copylen > AVAILABLE_BUFFER_LEN(outbuf))
			copylen = AVAILABLE_BUFFER_LEN(outbuf); //max length we can fit in the buffer
		if (copylen <= 0)
			return false; //no space
		memcpy(outbuf->buffer + outbuf->len, value, (unsigned int)copylen);
		outbuf->len += copylen;
		outbuf->buffer[outbuf->len - 1] = 0; //make sure it's null terminated
		return true;
	}
}

const char *qr2_registered_key_list[MAX_REGISTERED_KEYS] =
{
	"",				//0 is reserved
	"hostname",		//1
	"gamename",		//2
	"gamever",		//3
	"hostport",		//4
	"mapname",		//5
	"gametype",		//6
	"gamevariant",	//7
	"numplayers",	//8
	"numteams",		//9
	"maxplayers",	//10
	"gamemode",		//11
	"teamplay",		//12
	"fraglimit",	//13
	"teamfraglimit",//14
	"timeelapsed",	//15
	"timelimit",	//16
	"roundtime",	//17
	"roundelapsed",	//18
	"password",		//19
	"groupid",		//20
	"player_",		//21
	"score_",		//22
	"skill_",		//23
	"ping_",		//24
	"team_",		//25
	"deaths_",		//26
	"pid_",			//27
	"team_t",		//28
	"score_t",		//29
	"nn_groupid",	//30

					// Query From Master Only keys
					"country",		//31
					"region"		//32
};