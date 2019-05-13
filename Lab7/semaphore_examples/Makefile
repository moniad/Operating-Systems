all: semaphores.o semaphoresInPOSIX.o
	make clean
	make semaphores.o
	make semaphoresInPOSIX.o
	# make run

semaphoresInPOSIX.o: semaphoresInPOSIX.c
	gcc -Wall -o semaphoresInPOSIX.o semaphoresInPOSIX.c -g -lpthread -lrt

semaphores.o: semaphores.c
	gcc -Wall -o semaphores.o semaphores.c -g

clean:
	rm *.o
	ipcrm --all=sem
	ipcrm --all=shm