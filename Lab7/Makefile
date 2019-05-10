all: semaphores.o loader.o
	make clean
	# make semaphores.o
	make loader.o
	make loaders_manager.o
	make trucker.o
	# make run

semaphores.o: semaphores.c
	gcc -Wall -o semaphores.o semaphores.c -g

loader.o: loader.c
	gcc -Wall -o loader.o loader.c

loaders_manager.o: loaders_manager.c
	gcc -Wall -o loaders_manager.o loaders_manager.c

trucker.o: trucker.c
	gcc -Wall -o trucker.o trucker.c

# run: loader.o
# 	./loader.o

clean:
	rm *.o
	ipcrm --all=sem
	ipcrm --all=shm