$(warning ================================================)
$(warning $(CC))
$(warning ================================================)
CXXFLAGS  := -D_MCU_UPDATETOOL_CPUARCH_=1
OBJ += $(wildcard *.c)
INCLUDE  =  -I ./
TARGETNAME = lcm-api-tool

$(TARGETNAME):$(OBJ) 
	$(CC) $(CXXFLAGS) $^ -o $(TARGETNAME) $(INCLUDE)
clean:
	rm -rf $(TARGETNAME)
