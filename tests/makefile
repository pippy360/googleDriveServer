all: tests.out

tests.out: utils.o utils_tests.o
	gcc utils.o utils_tests.o -g -o tests.out

utils_tests.o: utils_tests.c
	gcc -c utils_tests.c

utils.o: ../utils.c
	gcc -c ../utils.c

