CC = gcc-10
CFLAGS = -o
DEPS = header.h
OBJ1  =  test_client.o  util.o
OBJ2  =  test_server.o  util.o

all: client server

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 

client: $(OBJ1)
		 $(CC) -o $@ $^ 

server: $(OBJ2)
		 $(CC) -o $@ $^ 

clean:
		rm -rf $(OBJ1) $(OBJ2) shellCC = gcc