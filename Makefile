SHELL=/bin/sh

OUTPUT=jpegrip
JPEGRIP_SRC=main.c jpegrip.c log.c
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

$(OUTPUT): x86_$(OUTPUT) arm_$(OUTPUT)
jpeghdr: x86_jpeghdr arm_jpeghdr

x86_$(OUTPUT): $(JPEGRIP_SRC)
arm_$(OUTPUT): $(JPEGRIP_SRC)
x86_jpeghdr: $(JPEGHDR_SRC)
arm_jpeghdr: $(JPEGHDR_SRC)

clean:
	-rm $(OUTPUT) x86_$(OUTPUT) arm_$(OUTPUT)
leaks: $(OUTPUT)
	leaks --atExit -- $(OUTPUT) sample.bin

else

$(OUTPUT): $(JPEGRIP_SRC)
clean:
	-rm $(OUTPUT)

endif # (OS)

debug: CFLAGS+=-g
debug: $(OUTPUT)

docker: clean
	docker build -t $(OUTPUT):latest .

fmt:
	clang-format -i $(JPEGRIP_SRC) $(HDR)
