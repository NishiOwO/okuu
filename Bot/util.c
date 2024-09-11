/* $Id$ */

#include "ok_util.h"

#include <string.h>
#include <stdlib.h>

char* ok_strcat(const char* a, const char* b) {
	char* str = malloc(strlen(a) + strlen(b) + 1);
	memcpy(str, a, strlen(a));
	memcpy(str + strlen(a), b, strlen(b));
	str[strlen(a) + strlen(b)] = 0;
	return str;
}

char* ok_strcat3(const char* a, const char* b, const char* c) {
	char* tmp = ok_strcat(a, b);
	char* str = ok_strcat(tmp, c);
	free(tmp);
	return str;
}

char* ok_strdup(const char* a) { return ok_strcat(a, ""); }
