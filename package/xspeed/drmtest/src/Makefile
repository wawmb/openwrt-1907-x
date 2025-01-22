CC = gcc
CFLAGS = -Wall
OBJS = drmtest.o
FLAGS += -I ./include

all: drmtest

%.o: %.c
	$(CC) $(FLAGS) $(CFLAGS) -c -o $@ $<

drmtest: $(OBJS)
	$(CC) -o $@ $(OBJS) -ldrm

clean:
	rm -f rbcfg *.o
