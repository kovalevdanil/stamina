all: stamina

stamina: stamina.o clist.o terminal.o abuf.o 
	gcc  stamina.o clist.o terminal.o abuf.o -o stamina -g -pthread

stamina.o: stamina.c stamina.h
	gcc -g -c stamina.c

clist.o: clist.c clist.h
	gcc -g -c clist.c

terminal.o: terminal.c terminal.h
	gcc -g -c terminal.c 

abuf.o: abuf.c abuf.h
	gcc -g -c abuf.c 

clean:
	rm -rf *.o