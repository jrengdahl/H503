# Master Makefile located at the top level of the project
# The purpose of this Makefile is to create a dummy .cyclo output
# for each .cpp or .c source file, then run the autogenerated
# ./Debug/makefile to build the project. This extra step is necessary
# because the autogenerated makefiles expect the STM32 GCC compiler
# which supports -fcyclomatic-complexity. My custom GCC does not support
# the plug which fenerates the .cyclo files, which results in a lot of
# warnings from make. Creating dummy cyclo files gets rid of those warnings. 

SPACE := $(null) $(null)

ROOT := $(realpath ..)
$(info ROOT = $(ROOT))

LIST  := $(shell find $(ROOT) \( -name '*.c' -o -name '*.cpp' \) -exec echo {} \; | sort -u)
$(info LIST = $(LIST))

TOUCHES := $(foreach file, $(LIST), $(basename $(patsubst $(ROOT)/%, %, $(file))).cyclo)
$(info TOUCHES = $(TOUCHES))

.PHONY: touch-files
touch-files:
	$(foreach file, $(TOUCHES), touch $(file);)
	
all: touch-files 
	$(MAKE) -f makefile $(MAKECMDGOALS)
