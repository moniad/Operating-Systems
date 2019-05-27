all: run

run: main.o
	./main.o 30 4 6 2 
	# >> Results.txt

main.o: main.c
	gcc -Wall -o main.o main.c -g -lpthread -lm
