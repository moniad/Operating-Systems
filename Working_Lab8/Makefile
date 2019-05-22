all: run

run: main.o
	# filter 1
	./main.o 1 block inFile.pgm filter1.txt outFile1.pgm > Times.txt
	./main.o 1 interleaved inFile.pgm filter1.txt outFile1.pgm >> Times.txt
	./main.o 2 block inFile.pgm filter1.txt outFile2.pgm >> Times.txt
	./main.o 2 interleaved inFile.pgm filter1.txt outFile2.pgm >> Times.txt
	./main.o 4 block inFile.pgm filter1.txt outFile3.pgm >> Times.txt
	./main.o 4 interleaved inFile.pgm filter1.txt outFile3.pgm >> Times.txt
	./main.o 8 block inFile.pgm filter1.txt outFile4.pgm >> Times.txt
	./main.o 8 interleaved inFile.pgm filter1.txt outFile4.pgm >> Times.txt

	./main.o 1 block inFile.pgm filter2.txt outFile11.pgm >> Times.txt
	./main.o 1 interleaved inFile.pgm filter2.txt outFile11.pgm >> Times.txt
	./main.o 2 block inFile.pgm filter2.txt outFile22.pgm >> Times.txt
	./main.o 2 interleaved inFile.pgm filter2.txt outFile22.pgm >> Times.txt
	./main.o 4 block inFile.pgm filter2.txt outFile33.pgm >> Times.txt
	./main.o 4 interleaved inFile.pgm filter2.txt outFile33.pgm >> Times.txt
	./main.o 8 block inFile.pgm filter2.txt outFile44.pgm >> Times.txt
	./main.o 8 interleaved inFile.pgm filter2.txt outFile44.pgm >> Times.txt

main.o: main.c
	gcc -Wall -o main.o main.c -lm -lpthread
