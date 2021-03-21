CC = gcc
CFLAGS = -o
DEPS = header.h
OBJ  =  execCommand.o shell.o  parseInput.o stringUtils.o

all: shell

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 

shell : $(OBJ)
		 $(CC) -o $@ $^ 

clean:
		rm -rf $(OBJ) shell
