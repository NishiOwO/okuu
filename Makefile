# $Id$

PLATFORM = generic
PWD = `pwd`

FLAGS = PWD=$(PWD) PLATFORM=$(PLATFORM)

.PHONY: all clean ./Bot

./Bot::
	$(MAKE) -C $@ $(FLAGS)

clean:
	$(MAKE) -C ./Bot $(FLAGS) clean
