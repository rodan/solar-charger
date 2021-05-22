
DEVICE:=MSP430F5510
SUBDIRS = drivers fatfs 
PROJ = proj

include ./Makefile.env

######################################
GCC_BIN_DIR      ?= $(GCC_DIR)/bin
GCC_INC_DIR      ?= $(GCC_DIR)/msp430-elf/include
######################################
CC               = $(GCC_BIN_DIR)/msp430-elf-gcc
GDB	             = $(GCC_BIN_DIR)/msp430-elf-gdb
######################################

### Machine flags
#
CC_CMACH	= -mmcu=msp430f5510
CC_DMACH	= -D__MSP430F5510__
### Build flags
#
# -fdata-sections, -ffunction-sections and -Wl,--gc-sections -Wl,-s
# are used for dead code elimination, see:
# http://gcc.gnu.org/ml/gcc-help/2003-08/msg00128.html
#
CFLAGS		+= $(CC_CMACH) $(CC_DMACH) -Wall
CFLAGS		+= -fshort-enums -Wl,-Map=output.map
LDFLAGS		:= -T $(LDDIR).ld -L $(LIB_DIR) -L $(GCC_EXTRA_INC_DIR) -mmcu=$(DEVICE) -mhwmult=f5series -g -Wl,--gc-sections

CFLAGS_REL	+= -Os -fdata-sections -ffunction-sections -fomit-frame-pointer
LDFLAGS_REL	+= -Wl,--gc-sections -Wl,-s

CFLAGS_DBG	+= -O1 -ggdb
LDFLAGS_DBG	+= -Wl,--gc-sections

# linker flags and include directories
INCLUDES         := -I $(GCC_EXTRA_INC_DIR) -I $(GCC_INC_DIR) -I $(DRIVERLIB) -I $(GLUELIB) -I $(GCC_CSS_INC_GCC_DIR) -I drivers/ -I fatfs/ -I ./ -I .

BASH := $(shell which bash || which bash)

.PHONY: all
.PHONY: clean
.PHONY: install
.PHONY: config
.PHONY: new_timestamps
.PHONY: depend
.PHONY: doc
.PHONY: tags
.PHONY: force

all: new_timestamps depend config.h tags $(PROJ).elf

#
# Build list of sources and objects to build
SRCS := $(wildcard *.c)
$(foreach subdir,$(SUBDIRS), \
	$(eval SRCS := $(SRCS) $(wildcard $(subdir)/*.c)) \
)
OBJS := $(patsubst %.c,%.o,$(SRCS))

#
# Dependencies rules
depend: $(PROJ).dep

$(PROJ).dep: $(SRCS)
	@echo "Generating dependencies.."
	@touch $@
	@makedepend $(INCLUDES) -Y -f $@ $^ &> /dev/null
	@rm -f $@.bak

#
# Append specific CFLAGS/LDFLAGS
DEBUG := $(shell grep "^\#define CONFIG_DEBUG" config.h)
ifeq ($(DEBUG),)
TARGET	:= RELEASE
CFLAGS	+= $(CFLAGS_REL)
LDFLAGS	+= $(LDFLAGS_REL)
else
TARGET	:= DEBUG
CFLAGS	+= $(CFLAGS_DBG)
LDFLAGS	+= $(LDFLAGS_DBG)
endif

# rebuild if CFLAGS changed, as suggested in:
# http://stackoverflow.com/questions/3236145/force-gnu-make-to-rebuild-objects-affected-by-compiler-definition/3237349#3237349
$(PROJ).cflags: force
	@echo "$(CFLAGS)" | cmp -s - $@ || echo "$(CFLAGS)" > $@

$(OBJS): $(PROJ).cflags
#
# Top rules

$(PROJ).elf: $(OBJS)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(LDFLAGS) $(OBJS) -o $@ && size $@

modinit.o: modinit.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -Wno-implicit-function-declaration \
		$(INCLUDES) -c $< -o $@

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

modinit.c:
	@echo "Please do a 'make config' first!" && false

config.h:
	@echo "Please do a 'make config' first!" && false

new_timestamps:
	@$(BASH) ./tools/update_rtca_now.sh

tags: $(SRCS)
	@echo "Generating tags .."
	@exuberant-ctags -R

install: $(PROJ).txt
	mspdebug olimex-iso-mk2 "prog $<"

clean: $(SUBDIRS)
	@for subdir in $(SUBDIRS); do \
		echo "Cleaning $$subdir .."; rm -f $$subdir/*.o; \
	done
	@rm -f *.o $(PROJ).{elf,txt,cflags,dep} output.map

doc:
	rm -rf doc/*
	doxygen Doxyfile

scan-build: clean
	scan-build --use-cc=$(CC) --analyzer-target=msp430-elf make PARSER=scan-build

cppcheck: clean
	@cppcheck --quiet --force --enable=warning --enable=performance --enable=information --enable=missingInclude -I $(DRIVERLIB) -I $(GLUELIB) -I ./ ./

-include $(PROJ).dep
