SHELL=/bin/sh

JPEGRIP=jpegrip
JPEGHDR=jpeghdr
JPEGRIP_SRC=main.c jpegrip.c log.c jpeg.c
JPEGHDR_SRC=jpeghdr.c jpeg.c log.c


# JPEGCF=$(shell pkg-config --cflags libjpeg)
# JPEGLF=$(shell pkg-config --libs libjpeg)

CFLAGS=-pedantic-errors -std=c89

.PHONY: fmt debug

OS=$(shell uname)

#
# darwin rules
#
%: x86_% arm_%
	lipo -create -output $@ $^
x86_%: %.c
	cc -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)
arm_%: %.c
	cc -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)
x86_%: CFLAGS+=-target x86_64-apple-macos10.12
arm_%: CFLAGS+=-target arm64-apple-macos11
# end of darwin rules

$(info building for $(OS))
ifeq ($(OS),Darwin)

$(JPEGRIP): x86_$(JPEGRIP) arm_$(JPEGRIP)
$(JPEGHDR): x86_$(JPEGHDR) arm_$(JPEGHDR)

x86_$(JPEGRIP): $(JPEGRIP_SRC)
arm_$(JPEGRIP): $(JPEGRIP_SRC)
x86_$(JPEGHDR): $(JPEGHDR_SRC)
arm_$(JPEGHDR): $(JPEGHDR_SRC)

clean:
	-rm $(JPEGRIP) x86_$(JPEGRIP) arm_$(JPEGRIP) $(JPEGHDR) x86_$(JPEGHDR) arm_$(JPEGHDR) *.jpg
	-rm -rf *.dSYM
leaks: $(JPEGRIP)
	leaks -atExit -- $(CURDIR)/$(JPEGRIP) sample.bin

else

$(JPEGRIP): $(JPEGRIP_SRC)
clean:
	-rm $(JPEGRIP)

endif # (OS)

debug: CFLAGS+=-g
debug: $(JPEGRIP)

docker: clean
	docker build -t $(JPEGRIP):latest .

fmt:
	clang-format -i $(JPEGRIP_SRC) $(HDR)
