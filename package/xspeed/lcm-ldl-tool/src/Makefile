$(warning ================================================)
$(warning $(CC))
$(warning ================================================)
all: lcm-ldl-tool

lcm-ldl-tool:
	$(CC) -D_MCU_UPDATETOOL_CPUARCH_=1 crc_external.c crc16.c drv_uart.c lcm_ldl_tool.c -o lcm-ldl-tool
clean:
	rm -f lcm-ldl-tool