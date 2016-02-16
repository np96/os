all:hello_world
hello_world: hello_world.o
hello_world.o:hello_world.c
	gcc -c $@ $<
hello_world:hello_world.o
	gcc -o  $@ $<
clean:
	rm -f *.o
