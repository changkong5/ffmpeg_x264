
#ARCH := arm64
#ARCH := arm32
ARCH := x86_64
#ARCH := android

ifeq ($(ARCH),arm64)
	CC = /opt/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
else ifeq ($(ARCH),arm32)
	CC = /opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
else ifeq ($(ARCH),x86_64)
	CC = gcc
else ifeq ($(ARCH),android)
	CC = /opt/android-ndk-r26b-linux/android-ndk-r26b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android34-clang
else 
	$(error "ARCH = $(ARCH)")
endif

#$(info "ARCH = $(ARCH)")
#$(info "CC = $(CC)")

Q = @
CFLAGS := 
LDFLAGS :=
CFLAGS += -include string.h
CFLAGS += -include strings.h

LDFLAGS += -lm 
LDFLAGS += -ldl
LDFLAGS += -lpthread

LDFLAGS += -lavcodec
LDFLAGS += -lavdevice
LDFLAGS += -lavfilter
LDFLAGS += -lavformat
LDFLAGS += -lavutil
LDFLAGS += -lswresample
LDFLAGS += -lswscale

#LDFLAGS += -lx264
LDFLAGS += -L$(CURDIR)/lib/x264/lib/$(ARCH)


#LDFLAGS += -ltinyalsa
#LDFLAGS += -L$(CURDIR)/lib/tinyalsa/lib/$(ARCH)

#LDFLAGS += -losip2
#LDFLAGS += -L$(CURDIR)/lib/osip/lib/$(ARCH)/osip

#LDFLAGS += -losipparser2
#LDFLAGS += -L$(CURDIR)/lib/osip/lib/$(ARCH)/osipparser2

#LDFLAGS += -leXosip2
#LDFLAGS += -L$(CURDIR)/lib/exosip/lib/$(ARCH)

#LDFLAGS += -ldbus-1
#LDFLAGS += -lglib-2.0
#LDFLAGS += -lell
#LDFLAGS += -lhistory
#LDFLAGS += -lreadline
#LDFLAGS += -lncurses

#LDFLAGS += -L$(CURDIR)/lib/dbus-1.0/lib/$(ARCH)
#LDFLAGS += -L$(CURDIR)/lib/glib-2.0/lib/$(ARCH)
#LDFLAGS += -L$(CURDIR)/lib/ell/lib/$(ARCH)
#LDFLAGS += -L$(CURDIR)/lib/readline/lib/$(ARCH)
#LDFLAGS += -L$(CURDIR)/lib/ncurses/lib/$(ARCH)

#CFLAGS += -D__GLIBC_USE_LIB_EXT2=1

#CFLAGS += -DSHELL=1

CFLAGS += -D__USE_GNU=1
CFLAGS += -DVERSION="\"V1.0\""

MAP := link.map
#CFLAGS += -D_GNU_SOURCE
LDFLAGS += -Wl,-Map=$(MAP)
LDFLAGS += -Wl,--sort-common
LDFLAGS += -Wl,--sort-section=name

INCLUDE := 
INCLUDE += -I$(CURDIR)/lib/x264/include

#INCLUDE += -I$(CURDIR)/lib/tinyalsa/include


#INCLUDE += -I$(CURDIR)/lib/osip/include
#INCLUDE += -I$(CURDIR)/lib/exosip/include

#INCLUDE += -I$(CURDIR)/lib/dbus-1.0/include
#INCLUDE += -I$(CURDIR)/lib/glib-2.0/include
#INCLUDE += -I$(CURDIR)/lib/ell/include
#INCLUDE += -I$(CURDIR)/lib/readline/include
#INCLUDE += -I$(CURDIR)/lib/ncurses/include

#INCLUDE += -I$(CURDIR)/core/b_core
#INCLUDE += -I$(CURDIR)/core/u_core
#INCLUDE += -I$(CURDIR)/core/w_core
#INCLUDE += -I$(CURDIR)/core/d_core

ROOT := ./
#ROOT += $(CURDIR)/core
#ROOT += $(CURDIR)/usr/cmd
#ROOT += $(CURDIR)/usr/interface
#ROOT += $(CURDIR)/usr/simple-cmd
#ROOT += $(CURDIR)/usr/simple-interface

SOURCES = $(shell find $(ROOT) -type f -name "*.c")
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

all:main

.PHONY: clean all

main: $(OBJECTS)
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	@rm -f $(OBJECTS) main $(MAP) yuv420p_w352xh288.h264

%.o: %.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@
	
%.i: %.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDE) -E $< -o $@	
	
