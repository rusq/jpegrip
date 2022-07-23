SHELL=/bin/sh

OUTPUT=jpegrip
SRC=main.c jpegrip.c log.c

JPEGCF=$(shell pkg-config --cflags libjpeg)
JPEGLF=$(shell pkg-config --libs libjpeg)

CFLAGS=-pedantic-errors -std=c89 $(JPEGCF)
LDFLAGS+=$(JPEGLF)

.PHONY: fmt debug

OS=$(shell uname)

$(info building for $(OS))
ifeq ($(OS),Darwin)
# $(OUTPUT): x86_$(OUTPUT) arm_$(OUTPUT)
# 	lipo -create -output $@ $^

# x86_%: %.c
$(OUTPUT): $(SRC)
	cc -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

# arm_%: %.c
# 	cc -o $@ $^ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS)

x86_$(OUTPUT): CFLAGS+=-target x86_64-apple-macos10.12
arm_$(OUTPUT): CFLAGS+=-target arm64-apple-macos11

x86_$(OUTPUT): $(SRC)

arm_$(OUTPUT): $(SRC)

clean:
	-rm $(OUTPUT) x86_$(OUTPUT) arm_$(OUTPUT)

leaks: $(OUTPUT)
	leaks --atExit -- $(OUTPUT) sample.bin
else
$(OUTPUT): $(SRC)
endif # (OS)

debug: CFLAGS+=-g
debug: $(OUTPUT)

docker: clean
	docker build -t $(OUTPUT):latest .

fmt:
	clang-format -i $(SRC) $(HDR)
