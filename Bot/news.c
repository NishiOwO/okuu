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

struct news_entry news_entry;

void ok_news_init(void){
	news_entry.from = NULL;
	news_entry.content = NULL;
}

int ok_news_read(const char* path){

	if(news_entry.from != NULL){
		free(news_entry.from);
		news_entry.from = NULL;
	}
	if(news_entry.content != NULL){
		free(news_entry.content);
		news_entry.content = NULL;
	}

	struct stat s;
	if(stat(path, &s) == 0){
		char* boundary = NULL;
		char* buffer = malloc(s.st_size + 1);
		buffer[s.st_size] = 0;
		FILE* f = fopen(path, "r");
		fread(buffer, s.st_size, 1, f);

		uint64_t i;
		bool newline = false;
		int incr = 0;
		char* l = malloc(1);
		l[0] = 0;
		bool header = true;
		bool ignore = false;
		bool bheader = false;
		for(i = 0; i < s.st_size; i++){
			if(buffer[i] == '\r'){
				if(buffer[i + 1] == '\n'){
					/* newline */
					i++;
					if(!header){
						char* line = malloc(i - 1 - incr + 1);
						line[i - 1 - incr] = 0;
						memcpy(line, buffer + incr, i - 1 - incr);

						if(strcmp(line, ".") == 0){
							free(line);
							break;
						}else{
							char* ln = line;
							if(line[0] == '.'){
								ln++;
							}

							if(news_entry.content == NULL){
								news_entry.content = malloc(1);
								news_entry.content[0] = 0;
							}

							if(boundary != NULL && strcmp(ln, boundary) == 0){
								bheader = true;
								ignore = true;
							}else if(boundary != NULL && bheader && strlen(ln) == 0){
								bheader = false;
								free(line);
								incr = i + 1;
								newline = true;
								continue;
							}else if(boundary != NULL && bheader){
								int j;
								for(j = 0; j < strlen(ln); j++){
									if(ln[j] == ':'){
										ln[j] = 0;
										if(strcasecmp(ln, "Content-Type") == 0){
											ignore = false;
											j++;
											for(; ln[j] != 0 && (ln[j] == ' ' || ln[j] == '\t'); j++);
											if(ln[j] != 0){
												char* v = ln + j;
												int k;
												for(k = 0; v[k] != 0; k++){
													if(v[k] == ';'){
														v[k] = 0;
														break;
													}
												}
												if(strcasecmp(v, "text/plain") == 0){
												}else{
													ignore = true;
												}
											}
										}
										break;
									}
								}
							}

							if(!ignore && !bheader){
								char* tmp = news_entry.content;
								news_entry.content = ok_strcat3(tmp, ln, "\n");
								free(tmp);
							}
						}

						free(line);
					}else if(newline){
						header = false;
					}else{
						char* line = malloc(i - 1 - incr + 1);
						line[i - 1 - incr] = 0;
						memcpy(line, buffer + incr, i - 1 - incr);

						char* last = ok_strdup(l);
						char* tmp = l;
						l = ok_strcat(tmp, line);
						free(tmp);
						bool al = false;
						if(('a' <= line[0] && line[0] <= 'z') || ('A' <= line[0] && line[0] <= 'Z')){
							free(l);
							l = ok_strdup(line);
							al = true;
						}
						if(al){
							char* ln = ok_strdup(l);
							int j;
							for(j = 0; ln[j] != 0; j++){
								if(ln[j] == ':'){
									char* key = ln;
									char* value = "";
									ln[j] = 0;
									j++;
									for(; ln[j] != 0 && (ln[j] == '\t' || ln[j] == ' '); j++);
									if(ln[j] != 0) value = ln + j;
									if(strcasecmp(key, "From") == 0){
										if(news_entry.from != NULL) free(news_entry.from);
										news_entry.from = ok_strdup(value);
									}else if(strcasecmp(key, "Content-Type") == 0){
										int k = 0;
										int incr2 = 0;
										for(k = 0; k <= strlen(value); k++){
											if(value[k] == ';' || value[k] == 0){
												char* attr = malloc(k - incr2 + 1);
												attr[k - incr2] = 0;
												memcpy(attr, value + incr2, k - incr2);

												int in;
												for(in = 0; attr[in] != 0; in++){
													if(attr[in] == '='){
														attr[in] = 0;

														if(strcasecmp(attr, "boundary") == 0){
															boundary = ok_strcat("--", attr + in + 1 + 1);
															boundary[strlen(attr + in + 1 + 1) - 1 + 2] = 0;
															ignore = true;
														}

														break;
													}
												}

												free(attr);
												k++;
												for(; value[k] != 0 && (value[k] == ' ' || value[k] == '\t'); k++);
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
				}else{
					newline = false;
				}
			}else{
				newline = false;
			}
		}
		free(l);

		free(buffer);
		if(boundary != NULL) free(boundary);
		return 0;
	}else{
		return 1;
	}
}
