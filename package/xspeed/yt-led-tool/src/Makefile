TARGET = Motorcomm_NIC_Burn_Helper

INC_DIR = -I . \
		  -I ./common \
		  -I ./project \

OPENWRT_LIB_DIR := $(STAGING_DIR)/usr/lib
$(warning ================================================)
$(warning $(OPENWRT_LIB_DIR))
$(warning $(CXX))
$(warning ================================================)
LIB_INPUT =-lpci

CCFLAGS += ${INC_DIR}

CPPCFLAGS += -std=c++11

CPPCFLAGS += ${INC_DIR}

DIRS := $(shell find . -type d)
DIRS += $(shell find ./common -type d)
DIRS += $(shell find ./project -type d)

# all .c files
CFILE := $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))

#all .cpp files
CPPFILE := $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))

# .c file to compile and link
OBJ_C := $(patsubst %.c,%.o,$(CFILE))

# .cpp file to compile and link
OBJ_CPP := $(patsubst %.cpp,%.o,$(CPPFILE))

# all file to compile and link
OBJ := $(OBJ_C) $(OBJ_CPP)

${TARGET}: ${OBJ}
	${CXX} ${CPPCFLAGS} -o $@ $^ ${LIB_INPUT}
	@echo "*************** Compile Motorcomm_NIC_Burn_Helper Success! ***************"

.SUFFIXES: .cpp .c
%.o: %.cpp
	${CXX} ${CPPCFLAGS} -c $< -o $@

%.o: %.c
	${CC} ${CCFLAGS} -c $< -o $@

.PHONY:clean
clean:
	rm -rf Motorcomm_NIC_Burn_Helper
	@echo "rm *.o:"
	@find . -name "*.o" -type f -print -exec rm -rf {} \;
	@find ./common/ -name "*.o" -type f -print -exec rm -rf {} \;
	@find ./project/ -name "*.o" -type f -print -exec rm -rf {} \;
