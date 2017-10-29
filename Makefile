.SUFFIXES:

### Options ###
DEBUG = 0

### External reference ###
LIBX265_PATH = libx265_2.5
BPG_PATH     = libbpg
FFMPEG_PATH  = ffmpeg-3.4-win32-dev

### Toolchains ###
AR  = ar
GCC = gcc
CXX = g++
RC  = windres
STRIP = strip

CPPFLAGS += \
    -DUSE_JCTVC \
    -DUSE_X265 \
    -D__STDC_CONSTANT_MACROS \
    -Isrc \
    -I$(FFMPEG_PATH)/include \
    -I$(BPG_PATH) \
    -std=gnu++11

CFLAGS += -Wall
LDFLAGS += -Wl,-Map,$@.map -Wl,--enable-stdcall-fixup
DEPFLAGS = -MMD -MF $@.d


ifeq ($(DEBUG),1)
  CFLAGS += -O0 -g
else
  CFLAGS += -O2 -march=i686
endif

ifeq ($(VERBOSE),1)
  V :=
else
  V := @
endif


### Search path
vpath %.cpp src
vpath %.rc  src
vpath %.def src
vpath %.a   $(LIBX265_PATH) $(BPG_PATH) $(FFMPEG_PATH)/lib


### Targets ###
.DEFAULT_GOAL = all

# XnView plug-in
.PHONY: xnview
xnview: obj/xnview/Xbpg.usr
MODULES += obj/xnview/Xbpg.usr
obj/xnview/Xbpg.usr: \
    obj/xnview/Xbpg.cpp.o \
    obj/xnview/Xbpg.rc.o  \
    xnview/Xbpg.def

# Susie plug-in
.PHONY: susie
susie: obj/susie/bpg.spi
MODULES += obj/susie/bpg.spi
obj/susie/bpg.spi: \
    obj/susie/spi_bpg.cpp.o \
    susie/spi00in.def

# Imagine plug-in
.PHONY: imagine
imagine: obj/imagine/bpg.plg
MODULES += obj/imagine/bpg.plg
obj/imagine/bpg.plg: \
    obj/imagine/bpg.cpp.o \
    obj/imagine/bpg.rc.o  \
    imagine/bpg.def


.PHONY: common
common: obj/libbpg_common.a
libbpg_common_SRCS = reader.cpp writer.cpp frame.cpp threadpool.cpp looptask.cpp dprintf.cpp sws_context.cpp av_util.cpp


include $(wildcard obj/*.d)
include $(wildcard $(addsuffix /*.d,$(notdir $(MODULES))))

.PHONY: all
all: $(MODULES)

.PHONY: clean
clean:
	rm -rf obj

.PHONY: libbpg libbpg-force
libbpg: $(BPG_PATH)/libbpg.a
libbpg-force \
$(BPG_PATH)/libbpg.a:
	cd $(BPG_PATH); make libbpg.a LIBX265_PATH=../$(LIBX265_PATH)

.PHONY: libbpg-clean
libbpg-clean:
	cd $(BPG_PATH); make clean LIBX265_PATH=../$(LIBX265_PATH)

# Output directory
obj $(dir $(MODULES)):
	@echo '[MKDIR] $@'
	@mkdir -p $@


src2obj = $(patsubst %,obj/%.o,$(1))

.SECONDEXPANSION:

.PRECIOUS: obj/%.cpp.o
obj/%.cpp.o: %.cpp | $$(@D)
	@echo '[CC] $< -> $@'
	$(V)$(CXX) -c $(CPPFLAGS) $(DEPFLAGS) $(CFLAGS) $(CXXFLAGS) $< -o $@

obj/%.rc.o: %.rc | $$(@D)
	@echo '[RC] $< -> $@'
	$(V)$(RC) $< $@

obj/%.a: $$(call src2obj,$$(%_SRCS)) | $$(@D)
	@echo '[AR] $@'
	$(V)$(AR) rcs $@ $^

# Link modules (DLL)
$(MODULES): obj/libbpg_common.a $(BPG_PATH)/libbpg.a libswscale.dll.a libx265.a
$(MODULES): | $$(@D)
	@echo '[LD] $@'
	$(V)$(CXX) -shared $(CPPFLAGS) $(CFLAGS) $(filter-out %.a,$^) $(filter %.a,$^) $(LDFLAGS) -o $@
ifneq ($(DEBUG),1)
	@echo '[STRIP] $@'
	$(V)$(STRIP) -s $@
endif
