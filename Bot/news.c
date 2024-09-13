/* $Id$ */

#define OK_NEWS_SRC
#include "ok_news.h"

#include "ok_util.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

extern char* nntpserver;
extern char* nntpuser;
extern char* nntppass;
extern char* nntppath;
extern char* nntpfrom;
extern char* nntpgroup;
extern char* nntpcount;
extern int nntpport;

struct news_entry news_entry;

void ok_close(int sock);

void ok_news_init(void) {
	news_entry.from = NULL;
	news_entry.content = NULL;
}

int ok_news_read(const char* path) {
	if(news_entry.from != NULL) {
		free(news_entry.from);
		news_entry.from = NULL;
	}
	if(news_entry.content != NULL) {
		free(news_entry.content);
		news_entry.content = NULL;
	}

	struct stat s;
	if(stat(path, &s) == 0) {
		char* boundary = NULL;
		char* buffer = malloc(s.st_size + 1);
		buffer[s.st_size] = 0;
		FILE* f = fopen(path, "rb");
		if(f == NULL){
			free(buffer);
			return 1;
		}
		fread(buffer, s.st_size, 1, f);

		uint64_t i;
		bool newline = false;
		int incr = 0;
		char* l = malloc(1);
		l[0] = 0;
		bool header = true;
		bool ignore = false;
		bool bheader = false;
		for(i = 0; i < s.st_size; i++) {
			if(buffer[i] == '\r') {
				if(buffer[i + 1] == '\n') {
					/* newline */
					i++;
					if(!header) {
						char* line = malloc(i - 1 - incr + 1);
						line[i - 1 - incr] = 0;
						memcpy(line, buffer + incr, i - 1 - incr);

						if(strcmp(line, ".") == 0) {
							free(line);
							break;
						} else {
							char* ln = line;
							if(line[0] == '.') {
								ln++;
							}

							if(news_entry.content == NULL) {
								news_entry.content = malloc(1);
								news_entry.content[0] = 0;
							}

							if(boundary != NULL && strcmp(ln, boundary) == 0) {
								bheader = true;
								ignore = true;
							} else if(boundary != NULL && bheader && strlen(ln) == 0) {
								bheader = false;
								free(line);
								incr = i + 1;
								newline = true;
								continue;
							} else if(boundary != NULL && bheader) {
								int j;
								for(j = 0; j < strlen(ln); j++) {
									if(ln[j] == ':') {
										ln[j] = 0;
										if(strcasecmp(ln, "Content-Type") == 0) {
											ignore = false;
											j++;
											for(; ln[j] != 0 && (ln[j] == ' ' || ln[j] == '\t'); j++)
												;
											if(ln[j] != 0) {
												char* v = ln + j;
												int k;
												for(k = 0; v[k] != 0; k++) {
													if(v[k] == ';') {
														v[k] = 0;
														break;
													}
												}
												if(strcasecmp(v, "text/plain") == 0) {
												} else {
													ignore = true;
												}
											}
										}
										break;
									}
								}
							}

							if(!ignore && !bheader) {
								char* tmp = news_entry.content;
								news_entry.content = ok_strcat3(tmp, ln, "\n");
								free(tmp);
							}
						}

						free(line);
					} else if(newline) {
						header = false;
					} else {
						char* line = malloc(i - 1 - incr + 1);
						line[i - 1 - incr] = 0;
						memcpy(line, buffer + incr, i - 1 - incr);

						char* last = ok_strdup(l);
						char* tmp = l;
						l = ok_strcat(tmp, line);
						free(tmp);
						bool al = false;
						if(('a' <= line[0] && line[0] <= 'z') || ('A' <= line[0] && line[0] <= 'Z')) {
							free(l);
							l = ok_strdup(line);
							al = true;
						}
						if(al) {
							char* ln = ok_strdup(l);
							int j;
							for(j = 0; ln[j] != 0; j++) {
								if(ln[j] == ':') {
									char* key = ln;
									char* value = "";
									ln[j] = 0;
									j++;
									for(; ln[j] != 0 && (ln[j] == '\t' || ln[j] == ' '); j++)
										;
									if(ln[j] != 0) value = ln + j;
									if(strcasecmp(key, "From") == 0) {
										if(news_entry.from != NULL) free(news_entry.from);
										news_entry.from = ok_strdup(value);
									} else if(strcasecmp(key, "Content-Type") == 0) {
										int k = 0;
										int incr2 = 0;
										for(k = 0; k <= strlen(value); k++) {
											if(value[k] == ';' || value[k] == 0) {
												char* attr = malloc(k - incr2 + 1);
												attr[k - incr2] = 0;
												memcpy(attr, value + incr2, k - incr2);

												int in;
												for(in = 0; attr[in] != 0; in++) {
													if(attr[in] == '=') {
														attr[in] = 0;

														if(strcasecmp(attr, "boundary") == 0) {
															boundary = ok_strcat("--", attr + in + 1 + 1);
															boundary[strlen(attr + in + 1 + 1) - 1 + 2] = 0;
															ignore = true;
														}

														break;
													}
												}

												free(attr);
												k++;
												for(; value[k] != 0 && (value[k] == ' ' || value[k] == '\t'); k++)
													;
												incr2 = k;
											}
										}
									}
								}
							}
							free(ln);
						}
						free(last);
						free(line);
					}
					incr = i + 1;
					newline = true;
				} else {
					newline = false;
				}
			} else {
				newline = false;
			}
		}
		free(l);

		free(buffer);
		fclose(f);
		if(boundary != NULL) free(boundary);
		return 0;
	} else {
		return 1;
	}
}

int ok_news_parse(int sock) {
	char c;
	int sta = 0;
	bool st = false;
	while(1) {
		if(recv(sock, &c, 1, 0) <= 0) {
			return -1;
		}
		if(c == '\n') break;
		if(!st) {
			if('0' <= c && c <= '9') {
				sta *= 10;
				sta += c - '0';
			} else if(c == ' ') {
				st = true;
			}
		}
	}
	return sta == 0 ? -1 : sta;
}

int ok_news_write(const char* nick, const char* message) {
	int nt_sock;
	struct sockaddr_in nt_addr;
	if((nt_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Socket creation failure: %s\n", strerror(errno));
		return 1;
	}

	bzero((char*)&nt_addr, sizeof(nt_addr));
	nt_addr.sin_family = PF_INET;
	nt_addr.sin_addr.s_addr = inet_addr(nntpserver);
	nt_addr.sin_port = htons(nntpport);

	int yes = 1;

	if(setsockopt(nt_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(yes)) < 0) {
		fprintf(stderr, "setsockopt failure");
		ok_close(nt_sock);
		return 1;
	}

	if(connect(nt_sock, (struct sockaddr*)&nt_addr, sizeof(nt_addr)) < 0) {
		fprintf(stderr, "Connection failure\n");
		ok_close(nt_sock);
		return 1;
	}

	int sta;

	sta = ok_news_parse(nt_sock);
	if(sta == 200 || sta == 201) {
		char construct[1024];
		if(nntpuser != NULL) {
			sprintf(construct, "AUTHINFO USER %s\r\n", nntpuser);
			send(nt_sock, construct, strlen(construct), 0);
			sta = ok_news_parse(nt_sock);
			if(sta != 381) {
				goto cleanup;
			}
		}
		if(nntppass != NULL) {
			sprintf(construct, "AUTHINFO PASS %s\r\n", nntppass);
			send(nt_sock, construct, strlen(construct), 0);
			sta = ok_news_parse(nt_sock);
			if(sta != 281) {
				goto cleanup;
			}
		}
		send(nt_sock, "MODE READER\r\n", 4 + 1 + 6 + 2, 0);
		sta = ok_news_parse(nt_sock);
		if(sta == 200 || sta == 201) {
			send(nt_sock, "POST\r\n", 4 + 2, 0);
			sta = ok_news_parse(nt_sock);
			if(sta == 340) {
				sprintf(construct, "From: %s\r\n", nntpfrom);
				send(nt_sock, construct, strlen(construct), 0);
				sprintf(construct, "Newsgroups: %s\r\n", nntpgroup);
				send(nt_sock, construct, strlen(construct), 0);
				sprintf(construct, "Subject: [IRC] Message from %s\r\n", nick);
				send(nt_sock, construct, strlen(construct), 0);
				sprintf(construct, "Content-Type: text/plain; charset=UTF-8\r\n", nick);
				send(nt_sock, construct, strlen(construct), 0);
				send(nt_sock, "\r\n", 2, 0);
				char c;
				int i;
				bool first = true;
				for(i = 0; message[i] != 0; i++) {
					if(message[i] == '\n') {
						send(nt_sock, "\r\n", 2, 0);
						first = true;
					} else {
						if(first && message[i] == '.') {
							send(nt_sock, message + i, 1, 0);
						}
						send(nt_sock, message + i, 1, 0);
						first = false;
					}
				}
				if(!first) send(nt_sock, "\r\n", 2, 0);
				send(nt_sock, ".\r\n", 3, 0);
				sta = ok_news_parse(nt_sock);
				if(sta != 240) goto cleanup;
				send(nt_sock, "QUIT\r\n", 6, 0);
				sta = ok_news_parse(nt_sock);
				if(sta != 205) goto cleanup;
			} else {
				goto cleanup;
			}
		} else {
			goto cleanup;
		}
	} else {
		goto cleanup;
	}

	ok_close(nt_sock);
	return 0;
cleanup:
	ok_close(nt_sock);
	return 1;
}
