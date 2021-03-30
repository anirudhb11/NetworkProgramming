CC = gcc
CFLAGS = -o
DEPS = message_queue.h
OBJ1  =  client.o
OBJ2  =  server.o

all: client server

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

client: $(OBJ1)
		 $(CC) -o $@ $^

server: $(OBJ2)
		 $(CC) -o $@ $^

clean:
		rm -rf $(OBJ1) $(OBJ2) client server