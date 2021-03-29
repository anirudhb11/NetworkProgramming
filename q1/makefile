CC = gcc-10
CFLAGS = -o
DEPS = header.h
OBJ  =  shell.o  parseInput.o stringUtils.o pipe.o

all: shell

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 

shell : $(OBJ)
		 $(CC) -o $@ $^ 

clean:
		rm -rf $(OBJ) shell
