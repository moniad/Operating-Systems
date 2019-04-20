all: Client.o Server.o
	make clean
	echo Run on two terminals
	make Client.o
	make Server.o
	# make run

Client.o: Client.c
	gcc -Wall -o Client.o Client.c -g

Server.o: Server.c
	gcc -Wall -o Server.o Server.c -g

run: Client.o Server.o
	./Client.o
	sleep 1
	./Server.o

clean:
	rm *.o
	ipcrm --all=msg 
	ipcrm --all=sem
	ipcrm --all=shm