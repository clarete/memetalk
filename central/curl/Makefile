all: build

SUBDIRS = examples

include $(shell git rev-parse --show-toplevel)/common.mk

$(call GEN_MEC_RULE)

SO := meme_curl.so
CODE := curl.me
BYTECODE := $(patsubst %.me,%.mec,$(CODE))

INCLUDES = -I$(ROOT_DIR)/src -I$(ROOT_DIR)/central/bindgen
CXXFLAGS = --std=c++11 -Wall $(INCLUDES)
LIBS = -lcurl

CPP_FILES = curl_prims.cpp
OBJS = $(CPP_FILES:%.cpp=%.o)

DIST_FILES = $(CODE) $(BYTECODE) $(SO)

%_prims.cpp: %.defs; $(call RUN_BINDGEN_CMD,$^) > $@ || rm $@
%.o: %.cpp; g++ $(CXXFLAGS) -fPIC -c -o $@ $^

$(SUBDIRS): build; $(MAKE) -C $@
$(OBJS): $(CPP_FILES)
$(SO): $(OBJS); g++ -shared -fPIC $(LIBS) $^ -o $@

build: $(SO) $(BYTECODE)
dist: $(DIST_FILES); $(call INSTALL_DIST_FILES,$^)
clean:; -rm -f $(BYTECODE) $(OBJS) $(SO) $(CPP_FILES)
