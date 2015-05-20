all:
	gcc test_main.c queue.c thread_pool.c -lpthread
clean:
	rm a.out
