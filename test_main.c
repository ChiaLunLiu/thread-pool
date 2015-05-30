#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread_pool.h"
#include <stdint.h>

void* print1(void * arg)
{
	char* c = (char*)arg;
//	printf("%s\n",__func__);
//	fprintf(stderr,"str=%s\n",c);
	sleep(1);
	free(c);
	return (void*)4;
}

int main()
{
	ssize_t s;
	uint64_t u;
	int i;
	void* data;
	thpool_t * thp;
	int cnt= 32;
	int sum = 0;
	thp = thread_pool_alloc(2);
	if(!thp)return 0;
	for(i = 0;i<cnt ; i++)
	thread_pool_schedule_task(thp, print1, strdup("hi"));
	while(sum<cnt){
	s = read(thp->efd, &u, sizeof(uint64_t));
	if (s != sizeof(uint64_t)){
		puts("error");
	}
	sum+=u;
	printf("task:%d\n",u);
        for(i = 0;  i< u ; i++){
            pthread_spin_lock(&thp->qlock);
                  data =  queue_pop( thp->q);
             pthread_spin_unlock(&thp->qlock);
//		printf("out:%d\n",(int)data);
	}
	}
	puts("destroy");
	
	thread_pool_destroy(thp);

//	while(1)sleep(1);
	return 0;
}
