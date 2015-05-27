obj_PATH = obj/
dll_NAME = Xbpg.usr

libx265_PATH = libx265/libx265.a
bpg_PATH = libbpg-0.9.5
libbpg_PATH = $(bpg_PATH)/libbpg.a

GCC = gcc
CXX = g++
RC = windres

CPPFLAGS += -DUSE_JCTVC -DUSE_X265 -I$(bpg_PATH)
CFLAGS += -Wall
LDFLAGS += 


ifeq ($(DEBUG),1)
CFLAGS += -O0 -g
else
CFLAGS += -O2
endif


dll_PATH = $(obj_PATH)$(dll_NAME)

dll_CPP += $(addprefix src/,reader.cpp writer.cpp Xbpg.cpp)
dll_RC += $(addprefix src/,Xbpg.rc)

dll_CPP_OBJ = $(patsubst src/%.cpp,$(obj_PATH)%.o,$(dll_CPP))
dll_RC_OBJ  = $(patsubst src/%.rc,$(obj_PATH)%.rc.o,$(dll_RC))
dll_OBJ += $(dll_CPP_OBJ) $(dll_RC_OBJ)

dll_DIR = $(sort $(dir $(dll_OBJ)))

.PHONY: all dll clean
.DEFAULT_GOAL = all

all: dll

dll: $(dll_PATH)

clean:
	rm -rf $(dll_DIR)


$(dll_PATH): $(dll_OBJ) src/Xbpg.def $(libbpg_PATH) $(libx265_PATH)
	$(CXX) -shared $(CFLAGS) $^ $(LDFLAGS) -o $@

$(dll_DIR):
	mkdir -p $@

$(dll_OBJ): | $(dll_DIR)

$(dll_CPP_OBJ): $(obj_PATH)%.o:src/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) $< -o $@

$(dll_RC_OBJ): $(obj_PATH)%.o:src/%
	$(RC) $< $@
