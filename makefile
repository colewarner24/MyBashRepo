CC = gcc   
CFLAGS = -g
LD = gcc   

all: mush2

mush2: mush2.o
	$(LD) -L ~pn-cs357/Given/Mush/libmush/lib64 -o mush2 mush2.o -lmush

mush2.o: mush2.c
	$(CC) $(CFLAGS) -I ~pn-cs357/Given/Mush/include -c -o mush2.o mush2.c

test: mush2
	./mush2
clean:
	rm -f *.o

