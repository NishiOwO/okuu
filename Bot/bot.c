/* $Id$ */

#include "ok_bot.h"

#include "ok_util.h"
#include "ok_news.h"

#include "ircfw.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <dirent.h>
#include <poll.h>

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#define OKUU_VERSION "1.00"

extern char* nntppath;
extern char* nntpcount;
extern char* nntpfrom;

extern char* ircserver;
extern char* ircchan;
extern char* ircpass;
extern char* ircuser;
extern char* ircnick;
extern char* ircreal;
extern int ircport;

int ok_sock;
struct sockaddr_in ok_addr;

void ok_close(int sock){
	close(sock);
}

void ok_bot_kill(int sig){
	fprintf(stderr, "Shutdown\n");
	ircfw_socket_send_cmd(ok_sock, NULL, "QUIT :Shutdown (Signal)");
	exit(1);
}

bool ok_is_number(const char* str) {
	int i;
	for(i = 0; str[i] != 0; i++) {
		if(!('0' <= str[i] && str[i] <= '9')) return false;
	}
	return true;
}

char* ok_null(const char* str){
	return str == NULL ? "(null)" : (char*)str;
}

int namesort(const struct dirent** a_, const struct dirent** b_){
	const struct dirent* a = *a_;
	const struct dirent* b = *b_;
	return atoi(a->d_name) - atoi(b->d_name);
}

int nodots(const struct dirent* d){
	return (strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, ".") == 0) ? 0 : 1;
}

void ok_bot(void){
	if((ok_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Socket creation failure\n");
		return;
	}

	bzero((char*)&ok_addr, sizeof(ok_addr));
	ok_addr.sin_family = PF_INET;
	ok_addr.sin_addr.s_addr = inet_addr(ircserver);
	ok_addr.sin_port = htons(ircport);

	int yes = 1;

	if(setsockopt(ok_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(yes)) < 0) {
		fprintf(stderr, "setsockopt failure");
		ok_close(ok_sock);
		return;
	}
	
	if(connect(ok_sock, (struct sockaddr*)&ok_addr, sizeof(ok_addr)) < 0){
		fprintf(stderr, "Connection failure\n");
		ok_close(ok_sock);
		return;
	}

	signal(SIGINT, ok_bot_kill);
	signal(SIGTERM, ok_bot_kill);

	char* construct = malloc(1025);

	if(ircpass != NULL && strlen(ircpass) > 0){
		sprintf(construct, "PASS :%s", ircpass);
		ircfw_socket_send_cmd(ok_sock, NULL, construct);
	}

	sprintf(construct, "USER %s %s %s :%s", ircuser, ircuser, ircuser, ircreal);
	ircfw_socket_send_cmd(ok_sock, NULL, construct);

	sprintf(construct, "NICK :%s", ircnick);
	ircfw_socket_send_cmd(ok_sock, NULL, construct);

	bool is_in = false;
	bool sendable = false;

	struct pollfd pollfds[1];
	pollfds[0].fd = ok_sock;
	pollfds[0].events = POLLIN | POLLPRI;

	while(1){
		int r = poll(pollfds, 1, 100);
		if(!(r > 0 && pollfds[0].revents & POLLIN)){
			/* 100ms sleep, technically */
			uint64_t count = 0;
			FILE* f = fopen(nntpcount, "rb");
			if(f != NULL){
				fread(&count, sizeof(count), 1, f);
				fclose(f);
			}
			struct dirent** list;
			int n = scandir(nntppath, &list, nodots, namesort);
			if(n >= 0){
				int i;
				for(i = 0; i < n; i++){
					if(!sendable){
						free(list[i]);
						continue;
					}
					if(count <= atoi(list[i]->d_name)){
						sprintf(construct, "%s/%s", nntppath, list[i]->d_name);
						if(ok_news_read(construct) == 0){
							if(strcmp(news_entry.from, nntpfrom) != 0){
								char* tmp = ok_strcat3("PRIVMSG ", ircchan, " :\x03" "07[USENET] ~ ");
								char* temp = ok_strcat3(tmp, news_entry.from, "\x03 ");
								free(tmp);
								int j;
								int incr = 0;
								for(j = 0;; j++){
									if(news_entry.content[j] == 0 || news_entry.content[j] == '\n'){
										char* line = malloc(j - incr + 1);
										line[j - incr] = 0;
										memcpy(line, news_entry.content + incr, j - incr);
	
										if(strlen(line) > 0){
											char* msg = ok_strcat(temp, line);
											ircfw_socket_send_cmd(ok_sock, NULL, msg);
											free(msg);
											usleep(1000 * 100); /* Sleep for 100ms */
										}
	
										free(line);
										incr = j + 1;
										if(news_entry.content[j] == 0) break;
									}
								}
								free(temp);
							}
						}else{
							fprintf(stderr, "Could not read %s\n", construct);
						}
					}
					free(list[i]);
				}
				count = atoi(list[i - 1]->d_name) + 1;
				free(list);
			}
			if(sendable){
				f = fopen(nntpcount, "wb");
				if(f != NULL){
					fwrite(&count, sizeof(count), 1, f);
					fclose(f);
				}
			}
			continue;
		}
		int st = ircfw_socket_read_cmd(ok_sock);
		if(st != 0){
			fprintf(stderr, "Bad response\n");
			return;
		}
		if(strlen(ircfw_message.command) == 3 && ok_is_number(ircfw_message.command)){
			int res = atoi(ircfw_message.command);
			if(!is_in && 400 <= res && res <= 599){
				fprintf(stderr, "Bad response\n");
				return;
			}else if(400 <= res && res <= 599){
				fprintf(stderr, "Ignored error: %d\n", res);
				continue;
			}
			if(res == 376){
				is_in = true;
				fprintf(stderr, "Login successful\n");
				sprintf(construct, "JOIN :%s", ircchan);
				ircfw_socket_send_cmd(ok_sock, NULL, construct);
			}else if(res == 331 || res == 332){
				sendable = true;
			}
		}else{
			if(strcasecmp(ircfw_message.command, "PING") == 0){
				fprintf(stderr, "Ping request\n");
				sprintf(construct, "PONG :%s", ok_null(ircfw_message.prefix));
				ircfw_socket_send_cmd(ok_sock, NULL, construct);
			}else if(strcasecmp(ircfw_message.command, "PRIVMSG") == 0){
				char* prefix = ircfw_message.prefix;
				char** params = ircfw_message.params;

				int len = 0;
				if(params != NULL) {
					int i;
					for(i = 0; params[i] != NULL; i++);
					len = i;
				}
				if(prefix != NULL && len == 2) {
					char* sentin = params[0];
					char* msg = params[1];
					char* nick = ok_strdup(prefix);
					int i;
					for(i = 0; nick[i] != 0; i++) {
						if(nick[i] == '!') {
							nick[i] = 0;
							break;
						}
					}
					if(msg[0] == 1 && msg[strlen(msg) - 1] == 1){
						/* CTCP */
						if(strcasecmp(msg, "\x01VERSION\x01") == 0){
							sprintf(construct, "NOTICE %s :\x01VERSION Okuu %s / IRC Frameworks %s: http://nishi.boats/okuu\x01", nick, OKUU_VERSION, IRCFW_VERSION);
							ircfw_socket_send_cmd(ok_sock, NULL, construct);
						}
					}else if(sentin[0] == '#'){
						/* This was sent in channel */
						if(ok_news_write(nick, msg) != 0){
							sprintf(construct, "PRIVMSG %s :Could not send the message to the USENET", sentin);
							ircfw_socket_send_cmd(ok_sock, NULL, construct);
						}
					}

					free(nick);
				}
			}else if(strcasecmp(ircfw_message.command, "ERROR") == 0){
				if(ircfw_message.params != NULL){
					int i;
					for(i = 0; ircfw_message.params[i] != NULL; i++) fprintf(stderr, "ERROR: %s\n", ircfw_message.params[i]);
				}
			}
		}
	}

	ircfw_socket_send_cmd(ok_sock, NULL, "QUIT :Shutdown");

	free(construct);
}
