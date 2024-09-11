/* $Id$ */

#include "ok_bot.h"

#include "ok_news.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

char* nntpserver;
char* nntpuser;
char* nntppass;
char* nntppath;
char* nntpgroup;
char* nntpfrom;
char* nntpcount;
int nntpport = 119;

char* ircserver;
char* ircchan;
char* ircuser;
char* ircnick;
char* ircreal;
char* ircpass;
int ircport = 6667;

int main() {
	printf("Okuu starting up\n");

	nntpserver = getenv("NNTPSERVER");
	nntpuser = getenv("NNTPUSER");
	nntppass = getenv("NNTPPASS");
	nntppath = getenv("NNTPPATH");
	nntpgroup = getenv("NNTPGROUP");
	nntpcount = getenv("NNTPCOUNT");
	nntpfrom = getenv("NNTPFROM");
	ircserver = getenv("IRCSERVER");
	ircchan = getenv("IRCCHAN");
	ircuser = getenv("IRCUSER");
	ircnick = getenv("IRCNICK");
	ircreal = getenv("IRCREAL");
	ircpass = getenv("IRCPASS");
	if(getenv("NNTPPORT") != NULL) {
		nntpport = atoi(getenv("NNTPPORT"));
	}
	if(getenv("IRCPORT") != NULL) {
		ircport = atoi(getenv("IRCPORT"));
	}
	bool bad = false;
	if(nntpserver == NULL) {
		fprintf(stderr, "Set NNTPSERVER\n");
		bad = true;
	}
	if(nntppath == NULL) {
		fprintf(stderr, "Set NNTPPATH\n");
		bad = true;
	}
	if(nntpcount == NULL) {
		fprintf(stderr, "Set NNTPCOUNT\n");
		bad = true;
	}
	if(nntpfrom == NULL) {
		fprintf(stderr, "Set NNTPFROM\n");
		bad = true;
	}
	if(nntpgroup == NULL) {
		fprintf(stderr, "Set NNTPGROUP\n");
		bad = true;
	}
	if(ircserver == NULL) {
		fprintf(stderr, "Set IRCSERVER\n");
		bad = true;
	}
	if(ircchan == NULL) {
		fprintf(stderr, "Set IRCCHAN\n");
		bad = true;
	}
	if(ircuser == NULL) {
		fprintf(stderr, "Set IRCUSER\n");
		bad = true;
	}
	if(ircnick == NULL) ircnick = ircuser;
	if(ircreal == NULL) ircreal = ircuser;
	if(bad) {
		return 1;
	}
	ok_news_init();
	ok_bot();
}
