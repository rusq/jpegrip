SHELL=/bin/sh

SRC=jpegrip.c log.c
HDR=log.h

.PHONY: fmt

jpegrip: $(SRC)

fmt:
	clang-format -i $(SRC) $(HDR)
