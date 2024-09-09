/* $Id$ */

#define IRCFW_SRC
#include "ircfw.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

struct ircfw_message ircfw_message;

char* ircfw_strcat(const char* a, const char* b) {
	char* str = malloc(strlen(a) + strlen(b) + 1);
	memcpy(str, a, strlen(a));
	memcpy(str + strlen(a), b, strlen(b));
	str[strlen(a) + strlen(b)] = 0;
	return str;
}

char* ircfw_strcat3(const char* a, const char* b, const char* c) {
	char* tmp = ircfw_strcat(a, b);
	char* str = ircfw_strcat(tmp, c);
	free(tmp);
	return str;
}

char* ircfw_strdup(const char* str) { return ircfw_strcat(str, ""); }

void ircfw_init(void) {
	ircfw_message.prefix = NULL;
	ircfw_message.params = NULL;
	ircfw_message.command = NULL;
}

void ircfw_parse_params(const char* str) {
	ircfw_message.params = malloc(sizeof(*ircfw_message.params));
	ircfw_message.params[0] = NULL;
	int i;
	int incr = 0;
	bool until_end = false;
	char* dup = ircfw_strdup(str);
	for(i = 0;; i++) {
		if(dup[i] == 0 || (!until_end && dup[i] == ' ')) {
			char oldc = dup[i];
			dup[i] = 0;

			char* param = ircfw_strdup(dup + incr);

			char** old_params = ircfw_message.params;
			int j;
			for(j = 0; old_params[j] != NULL; j++)
				;
			ircfw_message.params = malloc(sizeof(*ircfw_message.params) * (2 + j));
			for(j = 0; old_params[j] != NULL; j++) {
				ircfw_message.params[j] = old_params[j];
			}
			ircfw_message.params[j] = param;
			ircfw_message.params[j + 1] = NULL;
			free(old_params);

			incr = i + 1;
			if(oldc == 0) break;
		} else if(dup[i] == ':' && !until_end) {
			until_end = true;
			incr = i + 1;
		}
	}
	free(dup);
}

int ircfw_socket_send_cmd(int sock, const char* name, const char* cmd) {
	char* str = ircfw_strcat(cmd, "\r\n");
	if(name != NULL) {
		char* old = str;
		char* tmp = ircfw_strcat3(":", name, " ");
		str = ircfw_strcat(tmp, old);
		free(old);
		free(tmp);
	}
	int st = send(sock, str, strlen(str), 0);
	free(str);
	return st < 0 ? 1 : 0;
}

int ircfw_socket_read_cmd(int sock) {
	char c[2];
	c[1] = 0;
	char* str = malloc(1);
	str[0] = 0;
	bool err = false;
	bool end = false;
	while(1) {
		int s = recv(sock, c, 1, 0);
		if(s <= 0) {
			err = true;
			break;
		}
		if(c[0] == '\n') {
			end = true;
			break;
		} else if(c[0] != '\r') {
			char* tmp = str;
			str = ircfw_strcat(tmp, c);
			free(tmp);
		}
	}
	if(ircfw_message.prefix != NULL) free(ircfw_message.prefix);
	if(ircfw_message.params != NULL) {
		int i;
		for(i = 0; ircfw_message.params[i] != NULL; i++) {
			free(ircfw_message.params[i]);
		}
		free(ircfw_message.params);
	}
	if(ircfw_message.command != NULL) free(ircfw_message.command);
	ircfw_message.prefix = NULL;
	ircfw_message.params = NULL;
	ircfw_message.command = NULL;

	if(str[0] == ':') {
		int i;
		for(i = 0; str[i] != 0; i++) {
			if(str[i] == ' ') {
				str[i] = 0;
				ircfw_message.prefix = ircfw_strdup(str + 1);
				i++;
				int start = i;
				for(;; i++) {
					if(str[i] == ' ' || str[i] == 0) {
						char oldc = str[i];
						str[i] = 0;
						ircfw_message.command = ircfw_strdup(str + start);
						if(oldc != 0) {
							i++;
							ircfw_parse_params(str + i);
						}
						break;
					}
				}
				break;
			}
		}
	} else {
		int i;
		for(i = 0; str[i] != 0; i++) {
			if(str[i] == ' ' || str[i] == 0) {
				char oldc = str[i];
				str[i] = 0;
				ircfw_message.command = ircfw_strdup(str);
				if(oldc != 0) {
					i++;
					ircfw_parse_params(str + i);
				}
				break;
			}
		}
	}

	free(str);
	return err ? 1 : 0;
}
