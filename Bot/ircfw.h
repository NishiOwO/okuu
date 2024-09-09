/* $Id$ */

#ifndef __IRCFW_H__
#define __IRCFW_H__

#define IRCFW_VERSION "1.00"

struct ircfw_message {
	char* prefix;
	char* command;
	char** params;
};

void ircfw_init(void);
int ircfw_socket_send_cmd(int sock, const char* name, const char* cmd);
int ircfw_socket_read_cmd(int sock);

#ifndef IRCFW_SRC
extern struct ircfw_message ircfw_message;
#endif

#endif
