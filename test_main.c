#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread_pool.h"
#include <stdint.h>

void* print1(void * arg)
{
	char* c = (char*)arg;
	printf("%s\n",__func__);
//	fprintf(stderr,"str=%s\n",c);
	free(c);
	return (void*)4;
}

void process_callback_result(void* arg)
{
	
	printf("%s:%d\n",__func__, (int)arg);
}
int main()
{
	ssize_t s;
	uint64_t u;
	int i;
	void* data;
	thpool_t * thp;
	thp = thread_pool_alloc(1);
	for(i = 0;i<4 ; i++)
	thread_pool_schedule_task(thp, print1, strdup("hi"));

	s = read(thp->efd, &u, sizeof(uint64_t));
	if (s != sizeof(uint64_t)){
		puts("error");
	}
/*	printf("task:%d\n",u);
        for(i = 0;  i< u ; i++){
            pthread_spin_lock(&thp->qlock);
                  data =  queue_pop( thp->q);
             pthread_spin_unlock(&thp->qlock);
		printf("out:%d\n",(int)data);
	}
	puts("destroy");
*/
	thread_pool_destroy(thp);

//	while(1)sleep(1);
	return 0;
}
