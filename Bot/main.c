/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

char* nntpserver;
char* nntpuser;
char* nntppass;
int nntpport = 119;

char* ircserver;
int ircport = 6669;

int main(){
	printf("Okuu starting up\n");

	nntpserver = getenv("NNTPSERVER");
	nntpuser = getenv("NNTPUSER");
	nntppass = getenv("NNTPPASS");
	ircserver = getenv("IRCSERVER");
	if(getenv("NNTPPORT") != NULL){
		nntpport = atoi(getenv("NNTPPORT"));
	}
	if(getenv("IRCPORT") != NULL){
		ircport = atoi(getenv("IRCPORT"));
	}
	bool bad = false;
	if(nntpserver == NULL){
		fprintf(stderr, "Set NNTPSERVER\n");
		bad = true;
	}
	if(ircserver == NULL){
		fprintf(stderr, "Set IRCSERVER\n");
		bad = true;
	}
	if(bad){
		return 1;
	}
}
