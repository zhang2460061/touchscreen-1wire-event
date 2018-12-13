CC = arm-linux-gcc

v_mouse:main.o
	$(CC) -o v_mouse main.o 
main.o:main.c
	$(CC) -c main.c

.PRONY:clean
clean:
	rm -rf main.o v_mouse