
ARCH := arm64
#ARCH := arm32
#ARCH := x86_64
#ARCH := android

ifeq ($(ARCH),arm64)
	#CC = /opt/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
	CC = /opt/gcc-linaro-12.2.1-2022.12-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
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

# CURDIR := $(CURDIR)/../eXosip_lib

ROOT_DIR := $(CURDIR)

CURDIR := $(CURDIR)/..

Q = @
CFLAGS := 
LDFLAGS :=
CFLAGS += -include string.h
# CFLAGS += -include strings.h

# CFLAGS += -DFS_LL=1
# CFLAGS += -DFS_FS=1
# CFLAGS += -DFS_CUSE=1

# gcc -Wl,-verbose
# gcc -Wl,--whole-archive libmylib.a -Wl,--no-whole-archive -o output.o input.o
# gcc -Wl,--start-group file1.o file2.o -Wl,--end-group -o output.o input.o

#CFLAGS += -Os

CFLAGS += -flto

CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

LDFLAGS += -lm 
LDFLAGS += -lrt
LDFLAGS += -lpthread

LDFLAGS += -lglib-2.0
LDFLAGS += -ldbus-1
LDFLAGS += -lncurses
LDFLAGS += -lreadline
# LDFLAGS += -lintl

LDFLAGS += -lfuse3
LDFLAGS += -ljson-c
LDFLAGS += -lubox
LDFLAGS += -lblobmsg_json
LDFLAGS += -ljson_script
LDFLAGS += -lubus

LDFLAGS += -ltinyalsa

LDFLAGS += -leXosip2
LDFLAGS += -losip2
LDFLAGS += -losipparser2

LDFLAGS += -lssl -lcrypto
LDFLAGS += -lbcunit
LDFLAGS += -lbctoolbox
LDFLAGS += -lortp

LDFLAGS += -ldb
LDFLAGS += -levdev

LDFLAGS += -L$(CURDIR)/lib/x264/lib/$(ARCH)

LDFLAGS += -L$(CURDIR)/lib/libdb/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/libevdev/lib/$(ARCH)

LDFLAGS += -L$(CURDIR)/lib/bcunit/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/bctoolbox/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/ortp/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/openssl/lib/$(ARCH)

LDFLAGS += -L$(CURDIR)/lib/dbus-1.0/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/glib-2.0/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/ell/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/readline/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/ncurses/lib/$(ARCH)

LDFLAGS += -L$(CURDIR)/lib/fuse3/lib/$(ARCH)

LDFLAGS += -L$(CURDIR)/lib/json-c/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/libubox/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/ubus/lib/$(ARCH)

LDFLAGS += -L$(CURDIR)/lib/exosip/lib/$(ARCH)
LDFLAGS += -L$(CURDIR)/lib/osip/lib/$(ARCH)/osip
LDFLAGS += -L$(CURDIR)/lib/osip/lib/$(ARCH)/osipparser2

LDFLAGS += -L$(CURDIR)/lib/tinyalsa/lib/$(ARCH)

LDFLAGS += -llvgl
LDFLAGS += -llvgl_thorvg
LDFLAGS += -llvgl_demos
LDFLAGS += -llvgl_examples

LDFLAGS += -ldrm
LDFLAGS += -llz4
LDFLAGS += -lpng
LDFLAGS += -lz
LDFLAGS += -ljpeg

LDFLAGS += -lfreetype
LDFLAGS += -lrlottie

LDFLAGS += -lavcodec

LDFLAGS += -lavdevice
LDFLAGS += -lavfilter

LDFLAGS += -lavformat
LDFLAGS += -lavutil

LDFLAGS += -lswresample
LDFLAGS += -lswscale

LDFLAGS += -lpostproc
LDFLAGS += -lx264

LDFLAGS += -lffi
LDFLAGS += -lxkbcommon
LDFLAGS += -lwayland-client
LDFLAGS += -lwayland-cursor

CFLAGS += -L$(CURDIR)/lib/lvgl/lib/$(ARCH)
CFLAGS += -L$(CURDIR)/lib/libdrm/lib/$(ARCH)
CFLAGS += -L$(CURDIR)/lib/lz4/lib/$(ARCH)
CFLAGS += -L$(CURDIR)/lib/libpng/lib/$(ARCH)

CFLAGS += -L$(CURDIR)/lib/zlib/lib/$(ARCH)
CFLAGS += -L$(CURDIR)/lib/libjpeg-turbo/lib/$(ARCH)
CFLAGS += -L$(CURDIR)/lib/freetype/lib/$(ARCH)
CFLAGS += -L$(CURDIR)/lib/rlottie/lib/$(ARCH)

CFLAGS += -L$(CURDIR)/lib/FFmpeg/lib/$(ARCH)

CFLAGS += -L$(CURDIR)/lib/libffi/lib/$(ARCH)
CFLAGS += -L$(CURDIR)/lib/libxkbcommon/lib/$(ARCH)
CFLAGS += -L$(CURDIR)/lib/wayland/lib/$(ARCH)

#CFLAGS += -D_GNU_SOURCE
#CFLAGS += -D__GLIBC_USE_LIB_EXT2=1
#CFLAGS += -DSHELL=1
# CFLAGS += -D__USE_GNU=1
CFLAGS += -DVERSION="\"V1.0\""


LDFLAGS += -Wl,-Map=link.map
LDFLAGS += -Wl,--sort-common
LDFLAGS += -Wl,--sort-section=name


ifeq ($(ARCH),android)
	LDFLAGS += -Wl,--script=core/u_core/llvm_linker_script.lds
else
	LDFLAGS += -Wl,--script=$(CURDIR)/core/u_core/gcc_linker_script.lds
endif

INCLUDE := 

INCLUDE += -I$(CURDIR)/lib/x264/include

INCLUDE += -I$(CURDIR)/lib/dbus-1.0/include
INCLUDE += -I$(CURDIR)/lib/glib-2.0/include
INCLUDE += -I$(CURDIR)/lib/ell/include
INCLUDE += -I$(CURDIR)/lib/readline/include
INCLUDE += -I$(CURDIR)/lib/ncurses/include

INCLUDE += -I$(CURDIR)/lib/fuse3/include

INCLUDE += -I$(CURDIR)/lib/json-c/include
INCLUDE += -I$(CURDIR)/lib/json-c/include/json-c
INCLUDE += -I$(CURDIR)/lib/libubox/include
INCLUDE += -I$(CURDIR)/lib/ubus/include

INCLUDE += -I$(CURDIR)/lib/tinyalsa/include

INCLUDE += -I$(CURDIR)/lib/osip/include
INCLUDE += -I$(CURDIR)/lib/exosip/include

INCLUDE += -I$(CURDIR)/lib/openssl/include
INCLUDE += -I$(CURDIR)/lib/bcunit/include
INCLUDE += -I$(CURDIR)/lib/bctoolbox/include
INCLUDE += -I$(CURDIR)/lib/ortp/include

INCLUDE += -I$(CURDIR)/lib/libdb/include
INCLUDE += -I$(CURDIR)/lib/include/libevdev-1.0

INCLUDE += -I$(CURDIR)/core/b_core
INCLUDE += -I$(CURDIR)/core/u_core
INCLUDE += -I$(CURDIR)/core/w_core
INCLUDE += -I$(CURDIR)/core/d_core

INCLUDE += -I$(CURDIR)/ext/cJSON
INCLUDE += -I$(CURDIR)/ext/ortp
INCLUDE += -I$(CURDIR)/ext/eXosip
INCLUDE += -I$(CURDIR)/ext/fs_super


INCLUDE += -I$(CURDIR)/lib/lvgl/include
INCLUDE += -I$(CURDIR)/lib/libdrm/include
INCLUDE += -I$(CURDIR)/lib/lz4/include
INCLUDE += -I$(CURDIR)/lib/libpng/include

INCLUDE += -I$(CURDIR)/lib/zlib/include
INCLUDE += -I$(CURDIR)/lib/libjpeg-turbo/include
INCLUDE += -I$(CURDIR)/lib/freetype/include
INCLUDE += -I$(CURDIR)/lib/rlottie/include
INCLUDE += -I$(CURDIR)/lib/FFmpeg/include

INCLUDE += -I$(CURDIR)/lib/libffi/include
INCLUDE += -I$(CURDIR)/lib/libxkbcommon/include
INCLUDE += -I$(CURDIR)/lib/wayland/include

ROOT := 
ROOT += $(ROOT_DIR)
ROOT += $(CURDIR)/core

SOURCES = $(shell find $(ROOT) -type f -name "*.c")
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))

# CFLAGS += -v

all:args_main

.PHONY: clean all

args_main: $(OBJECTS)
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	@rm -f $(OBJECTS) args_main link.map yuv420p_w640xh480.*

%.o: %.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@ $(LDFLAGS)
	
%.i: %.c
	$(Q)$(CC) $(CFLAGS) $(INCLUDE) -E $< -o $@ $(LDFLAGS)

	
