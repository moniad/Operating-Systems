all: run

run: main.o
	./main.o 10 4 3 2

main.o: main.c
	gcc -Wall -o main.o main.c -g -lpthread -lm -D_FORTIFY_SOURCE=2
