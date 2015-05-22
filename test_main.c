#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread_pool.h"

void* print1(void * arg)
{
	char* c = (char*)arg;
	fprintf(stderr,"hi\n");
	fprintf(stderr,"str=%s\n",c);
	free(c);
	return (void*)4;
}

void process_callback_result(void* arg)
{
	
	printf("%s:%d\n",__func__, (int)arg);
}
int main()
{
	thpool_t * thp;
	thp = thread_pool_alloc(2,process_callback_result);
	thread_pool_schedule_task(thp, print1, strdup("hi"));

	thread_pool_destroy(thp);

//	printf("done\n");
//	while(1)sleep(1);
	return 0;
}
