all: loader.o loaders_manager.o trucker.o
	make clean
	make loader.o
	make loaders_manager.o
	make trucker.o
	# make run

loader.o: loader.c
	gcc -Wall -o loader.o loader.c -g

loaders_manager.o: loaders_manager.c
	gcc -Wall -o loaders_manager.o loaders_manager.c -g

trucker.o: trucker.c
	gcc -Wall -o trucker.o trucker.c -g

# run: loader.o
# 	./loader.o

clean:
	rm *.o
	ipcrm --all=sem
	ipcrm --all=shm