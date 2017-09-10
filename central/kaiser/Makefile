all: build

include $(shell git rev-parse --show-toplevel)/common.mk

$(call GEN_OMETA_TR_RULE)
$(call GEN_KAISER_TR_RULE)
$(call GEN_MEC_RULE)

CODE := kaiser_base.me gen_parser.me
BYTECODE := $(patsubst %.me,%.mec,$(CODE))

OMETA_GRAMMARS := kaiser.g kaiser_tr.g
KAISER_GRAMMARS := meme.k

GCODE := $(patsubst %.g,%.me,$(OMETA_GRAMMARS))
GBCODE := $(patsubst %.g,%.mec,$(OMETA_GRAMMARS))

KCODE := $(patsubst %.k,%.me,$(KAISER_GRAMMARS))
KBCODE := $(patsubst %.k,%.mec,$(KAISER_GRAMMARS))

build: $(BYTECODE) $(GCODE) $(GBCODE) $(KCODE) $(KBCODE)
clean:; -rm -f $(BYTECODE) $(GBCODE)