OUT_DIR := $(TOP_DIR)sbin/
objs_tools_o = $(patsubst %.c,%.o,$(wildcard ./tools/*.c))
objs_tools   = $(patsubst %.c,%,$(wildcard ./tools/*.c))

FLAGS +=-lm

all:clean $(OUT_DIR) tools

tools: $(objs_comm) $(objs_tools)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(objs_tools):%:%.o

	$(CC) -Wall $(FLAGS) $(CFLAGS) $(LDFLAGS) $(LIB_SO) -g -o $@ $@.o $(objs_comm) $(LDLIBS) 
	cp $@ $(OUT_DIR)

%.o: %.c
	$(CC) -Wall $(FLAGS) $(CFLAGS)  -c $^ -o $@

clean:
	rm -rf $(OUT_DIR) $(objs_tools_o)