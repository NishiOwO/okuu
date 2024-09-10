/* $Id$ */

#ifndef __OK_NEWS_H__
#define __OK_NEWS_H__

struct news_entry {
	char* from;
	char* content;
};

void ok_news_init(void);
int ok_news_read(const char* path);

#ifndef OK_NEWS_SRC
extern struct news_entry news_entry;
#endif

#endif
