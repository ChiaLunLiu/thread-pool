all:
	gcc test_main.c queue.c thread_pool.c -lpthread -g
library:
	gcc -c -O2 -fPIC queue.c
	gcc -c -O2 -fPIC thread_pool.c
	gcc -shared -o libthread_pool.so queue.o thread_pool.o
clean:
	rm a.out
