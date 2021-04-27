CC = gcc
CFLAGS = -o
DEPS = prefork_server.h
OBJ  =  prefork_server.o

all: prefork_server

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

prefork_server : $(OBJ)
		 $(CC) -o $@ $^

clean:
		rm -rf $(OBJ) prefork_server
