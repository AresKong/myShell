myshell: myshell.o utility.o
	gcc myshell.o utility.o -o myshell

myshell.o: myshell.c myshell.h
	gcc -c myshell.c -std=gnu99

utility.o: utility.c myshell.h
	gcc -c utility.c -std=gnu99
