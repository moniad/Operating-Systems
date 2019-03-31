all:
	make checking.o
	# make testOfReturn
	
checking.o: checking.c
	echo Jedynie kompilacja w Makefile/u, nic więcej!!!
	gcc -Wall -o checking.o checking.c -g
testOfReturn.o: testOfReturn.c
	gcc -Wall -o testOfReturn.o testOfReturn.c
	
testOfReturn: testOfReturn.o
	./testOfReturn.o

clean:
	rm *.o