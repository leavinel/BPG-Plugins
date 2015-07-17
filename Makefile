### External reference ###
imagine_NAME = imagine/bpg.plg
susie_NAME = susie/bpg.spi

libx265_LIB = libx265/libx265.a
bpg_PATH = libbpg-0.9.5
libbpg_LIB = $(bpg_PATH)/libbpg.a

### Toolchains ###
AR = ar
GCC = gcc
CXX = g++
RC = windres

CPPFLAGS += -DUSE_JCTVC -DUSE_X265 -I$(bpg_PATH) -Isrc
CFLAGS += -Wall
LDFLAGS += 

ifeq ($(DEBUG),1)
CFLAGS += -O0 -g
else
CFLAGS += -O2 -march=i686
endif


### Utility functions ###
# Combine patsubst & filter functions
filter-subst = $(patsubst $2,$3,$(filter $2, $1))

# Convert source files to object files
src2obj = \
	$(call filter-subst,$1,src/%.cpp,obj/%.cpp.o) \
	$(call filter-subst,$1,src/%.rc,obj/%.rc.o) \
	$(filter %.def,$1)

# Get directory list of input files
out-dirs = $(sort $(filter obj/%,$(dir $1)))

# Macros of defining sub-module variables & rules
define end-module-eval
$(MODULE_NAME)_SRC    = $$(addprefix src/$(MODULE_NAME)/,$(MODULE_SRC))
$(MODULE_NAME)_H      = $$(wildcard src/$(MODULE_NAME)/*.h)
$(MODULE_NAME)_OBJ    = $$(call src2obj,$$($(MODULE_NAME)_SRC))
$(MODULE_NAME)_OUTDIR = $$(call out-dirs,$$($(MODULE_NAME)_OBJ))
$(MODULE_NAME)_OUT2   = obj/$(MODULE_NAME)/$(MODULE_OUT)
MODULES += $(MODULE_NAME)
MODULES_OUT += $$($(MODULE_NAME)_OUT2)
MODULES_OUTDIR += $$($(MODULE_NAME)_OUTDIR)
$(MODULE_NAME): $$($(MODULE_NAME)_OUT2)
$$($(MODULE_NAME)_OBJ): | $$($(MODULE_NAME)_H) $$($(MODULE_NAME)_OUTDIR)
$$($(MODULE_NAME)_OUT2): $$($(MODULE_NAME)_OBJ)
MODULE_NAME :=
MODULE_SRC :=
MODULE_OUT :=
endef

# Called at the end of module.mk
end-module = $(eval $(end-module-eval))


### Variables ###
# Search & include sub-modules
MODULES_MK = $(shell ls src/*/module.mk)
$(foreach m,$(MODULES_MK),$(eval include $(m)))

# Common parts
common_SRC    = $(addprefix src/, reader.cpp writer.cpp dprintf.cpp)
common_H      = $(wildcard src/*.h)
common_OBJ    = $(call src2obj,$(common_SRC))
common_LIB    = obj/libbpg-common.a
common_OUTDIR = $(call out-dirs,$(common_OBJ))

MODULES += common
MODULES_OUTDIR += $(common_OUTDIR)
.PHONY: common
common: $(common_LIB)
$(common_OBJ): | $(common_H) $(common_OUTDIR)
$(common_LIB): $(common_OBJ)


### Targets ###
.PHONY: all clean $(MODULES)
.DEFAULT_GOAL = all

all: $(MODULES)

clean:
	rm -rf obj

$(MODULES_OUT): $(common_LIB) $(libbpg_LIB) $(libx265_LIB)

$(MODULES_OUT):
	$(CXX) -static -shared $(CFLAGS) $^ $(LDFLAGS) -o $@

$(MODULES_OUTDIR):
	mkdir -p $@

obj/%.o: $(common_H)

obj/%.a:
	$(AR) rcs $@ $^

obj/%.c.o: src/%.cpp
	$(GCC) -c $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $< -o $@

obj/%.cpp.o: src/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $< -o $@

obj/%.rc.o: src/%.rc
	$(RC) $< $@
