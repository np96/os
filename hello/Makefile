dynamic_run:dynamic
	gcc -c main.c -o main.o
	gcc main.o -o main -L. -lhello

static_run:static
	gcc -c main.c -o main.o
	gcc main.o -o main -L. -lhello

static: hello.o
	gcc -c hello.c -o hello.o
	ar rcs libhello.a hello.o

dynamic: hello.o
	gcc -fPIC -c hello.c -o hello.o
	gcc -shared hello.o -o libhello.so
	export LD_LIBRARY_PATH=.

	
