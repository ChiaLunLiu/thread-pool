all:
	gcc test_main.c queue.c thread_pool.c -lpthread -g
clean:
	rm a.out
