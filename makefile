SOURCES=$(wildcard *.c)
HEADERS=$(SOURCES:.c=.h)
NAME=proj
#FLAGS=-DDEBUG -g
FLAGS=-g -Wall

default: main

all: main tags

main: $(SOURCES) $(HEADERS) makefile
	mpicc $(SOURCES) $(FLAGS) -o $(NAME) -Wall

clear: clean

clean:
	rm $(NAME)

tags: ${SOURCES} ${HEADERS}

run: main makefile tags
	mpirun -oversubscribe -np 8 ./$(NAME)
