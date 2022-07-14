SHELL=/bin/sh

SRC=jpegrip.c log.c

.PHONY: fmt

ifeq ($(OS),Darwin)
x86_%: %.c
	cc -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

arm_%: %.c
	cc -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

x86_jpegrip: CFLAGS=-target x86_64-apple-macos10.12
arm_jpegrip: CFLAGS=-target arm64-apple-macos11


jpegrip: x86_jpegrip arm_jpegrip
	lipo -create -output $@ $^

x86_jpegrip: $(SRC)

arm_jpegrip: $(SRC)
else # not Darwin
jpegrip: $(SRC)
endif # (OS)

fmt:
	clang-format -i $(SRC) $(HDR)
