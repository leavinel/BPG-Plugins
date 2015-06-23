obj_PATH = obj/
xnview_NAME = xnview/Xbpg.usr
imagine_NAME = imagine/bpg.plg

libx265_PATH = libx265/libx265.a
bpg_PATH = libbpg-0.9.5
libbpg_PATH = $(bpg_PATH)/libbpg.a

GCC = gcc
CXX = g++
RC = windres

CPPFLAGS += -DUSE_JCTVC -DUSE_X265 -I$(bpg_PATH) -Isrc
CFLAGS += -Wall
LDFLAGS += 


ifeq ($(DEBUG),1)
CFLAGS += -O0 -g
else
CFLAGS += -O2
endif


# Utility functions
filter-subst=$(patsubst $2,$3,$(filter $2, $1))

src2obj=\
$(call filter-subst,$1,src/%.cpp,obj/%.cpp.o)\
$(call filter-subst,$1,src/%.rc,obj/%.rc.o)\
$(filter %.def,$1)

out-dirs=$(sort $(filter obj/%,$(dir $1)))

# Variables
xnview_OUT=obj/$(xnview_NAME)
imagine_OUT=obj/$(imagine_NAME)

common_SRC+=$(addprefix src/, reader.cpp writer.cpp dprintf.cpp)
common_OBJ=$(call src2obj,$(common_SRC))
common_H=$(wildcard src/*.h)

xnview_SRC+=$(addprefix src/xnview/,Xbpg.cpp Xbpg.rc Xbpg.def)
xnview_OBJ=$(call src2obj,$(xnview_SRC)) $(common_OBJ)
xnview_OUTDIR=$(call out-dirs,$(xnview_OBJ))

imagine_SRC+=$(addprefix src/imagine/,bpg.cpp bpg.rc bpg.def)
imagine_OBJ=$(call src2obj,$(imagine_SRC)) $(common_OBJ)
imagine_OUTDIR=$(call out-dirs,$(imagine_OBJ))


# Targets
.PHONY: all xnview imagine clean
.DEFAULT_GOAL = all

all: xnview imagine

clean:
	rm -rf obj

xnview: $(xnview_OUT)
imagine: $(imagine_OUT)

$(xnview_OBJ): | $(xnview_OUTDIR)
$(xnview_OUT): $(xnview_OBJ)

$(imagine_OBJ): | $(imagine_OUTDIR)
$(imagine_OUT): $(imagine_OBJ)

$(xnview_OUT) $(imagine_OUT): $(libbpg_PATH) $(libx265_PATH)

$(xnview_OUT) $(imagine_OUT):
	$(CXX) -static -shared $(CFLAGS) $^ $(LDFLAGS) -o $@

$(sort $(xnview_OUTDIR) $(imagine_OUTDIR)):
	mkdir -p $@

obj/%.o: $(common_H)

obj/%.cpp.o: src/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $< -o $@

obj/%.rc.o: src/%.rc
	$(RC) $< $@
