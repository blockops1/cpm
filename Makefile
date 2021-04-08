cpmRun: diskSimulator.o  cpmfsys.o fsystest.o
	gcc -g -Wall -Werror -o cpmRun diskSimulator.o cpmfsys.o fsystest.o 

diskSimulator.o: diskSimulator.c diskSimulator.h
	gcc -g -Wall -Werror -c diskSimulator.c

cpmfsys.o: cpmfsys.h cpmfsys.c 
	gcc -g -Wall -Werror -c cpmfsys.c  

fsystest.o: fsystest.c
	gcc -g -Wall -Werror -c fsystest.c

all:
	cpmRun

clean: 
	rm *.o 

