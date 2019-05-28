all: run

run: main.o
	./main.o 10 3 3 3
	# >> Results.txt

main.o: main.c
	gcc -Wall -o main.o main.c -g -lpthread -lm
