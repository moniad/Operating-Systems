all: trucker.o loader.o
# loaders_manager.o
	make clean
	make loader.o
	# make loaders_manager.o
	make trucker.o
	# make run

loader.o: loader.c
	gcc -Wall -o loader.o loader.c -g -lpthread -lrt

# loaders_manager.o: loaders_manager.c
# 	gcc -Wall -o loaders_manager.o loaders_manager.c -g -lpthread -lrt

trucker.o: trucker.c
	gcc -Wall -o trucker.o trucker.c -g -lpthread -lrt

# run: loader.o
# 	./loader.o

clean:
	rm *.o

# how to run: ./trucker.o 2 4 30 on one terminal,
# ./loader.o 3
# 14 2 <- if you use -1 instead, the worker will put packages on belt in an infinite loop
# 15 4
# 16 3 on second one
