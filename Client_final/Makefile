all: client.o func_lib.o
	gcc -o client client.o func_lib.o
client.o: client.c client.h
	gcc -c client.c
func_lib.o: func_lib.c func_lib.h
	gcc -c func_lib.c
