CC = gcc
CFLAGS = -o
DEPS = rtt.h
OBJ  =  rtt.o

all: rtt

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

rtt : $(OBJ)
		 $(CC) -o $@ $^

clean:
		rm -rf $(OBJ) rtt
