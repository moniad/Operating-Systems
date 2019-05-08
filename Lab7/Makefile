all: semaphores.o
	make clean
	make semaphores.o
	make run

semaphores.o: semaphores.c
	gcc -Wall -o semaphores.o semaphores.c -g

run: semaphores.o
	./semaphores.o

clean:
	rm *.o
	ipcrm --all=sem
	ipcrm --all=shm