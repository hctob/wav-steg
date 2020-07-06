all:	main

main:	part2.o
	g++ -Wall -Wextra -pedantic -fopenmp -mavx -g main.o -o wav-steg

part2.o:	main.cpp
	g++ -Wall -Wextra -pedantic -fopenmp -mavx -g -c main.cpp
clean:
	rm -f *.o wav-steg png-steg
