# $Id$

PLATFORM = generic
PWD = `pwd`

FLAGS = PWD=$(PWD) PLATFORM=$(PLATFORM)

.PHONY: all format clean ./Bot

./Bot::
	$(MAKE) -C $@ $(FLAGS)

format:
	clang-format -i --verbose `find . -name "*.c" -or -name "*.h"`

clean:
	$(MAKE) -C ./Bot $(FLAGS) clean
