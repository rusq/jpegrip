SHELL=/bin/sh

SRC=main.c jpegrip2.c log.c

CFLAGS=-pedantic-errors -std=c89

.PHONY: fmt debug

OS=$(shell uname)

$(info building for $(OS))
ifeq ($(OS),Darwin)
jpegrip: x86_jpegrip arm_jpegrip
	lipo -create -output $@ $^

x86_%: %.c
	cc -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

arm_%: %.c
	cc -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

x86_jpegrip: CFLAGS+=-target x86_64-apple-macos10.12
arm_jpegrip: CFLAGS+=-target arm64-apple-macos11

x86_jpegrip: $(SRC)

arm_jpegrip: $(SRC)

clean:
	-rm jpegrip x86_jpegrip arm_jpegrip
else
jpegrip: $(SRC)
endif # (OS)

debug: CFLAGS+=-g
debug: jpegrip

docker: clean
	docker build -t jpegrip:latest .

fmt:
	clang-format -i $(SRC) $(HDR)
