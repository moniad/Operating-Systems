all:
	make sender.o
	make catcher.o
	
sender.o: sender.c
	gcc -Wall -o sender.o sender.c -g

catcher.o: catcher.c
	gcc -Wall -o catcher.o catcher.c -g

clean:
	rm *.o