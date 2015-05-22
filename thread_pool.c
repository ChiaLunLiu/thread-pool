#include "thread_pool.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct _qelement_t
{
	void*(*task)(void* arg); /* task function should free arg */
	void* arg;
}qelement_t;

/*
 *  after kill_task is called, we should not insert any task to queue
 */
void* kill_task(void* arg)
{
	thnode_t* th = (thnode_t*)arg;
	/* pool would free the thread resource */
	th->active = 0;
	return NULL;
}


static void* do_task(void* arg)
{
	int i;
	int s;
	uint64_t u;
	void* r;
	thnode_t* th = (thnode_t*)arg;		
	qelement_t * qe;
	while(th->active){
		s = read(th->efd, &u, sizeof(uint64_t));
        	if (s != sizeof(uint64_t)) handle_error("read");
        	for(i = 0; th->active && i< u ; i++){
			pthread_spin_lock(&th->qlock);
				qe = (qelement_t*) queue_pop( th->q); 
			pthread_spin_unlock(&th->qlock);
			if(!qe){
						/* this shouldn't happen */
						handle_error("queue is null");
			}
			else{
				if(qe->task == NULL) fprintf(stderr,"null task\n");
				else{
					r=qe->task(qe->arg);
					if(r && th->result_cb){
						th->result_cb(r);
					}			
				}
				free(qe);
			}
		}
	}	
	return NULL;
}
thnode_t* thread_create(void (*result_cb)(void* arg) )
{
	int s;
	thnode_t* th;
	th = malloc( sizeof( thnode_t ));
	if(!th) handle_error("malloc thnode_t");
	
	th->q = queue_alloc();
	if(!th->q)handle_error("malloc queue_t");
		
	th->efd = eventfd(0,0);
	if(th->efd == -1) handle_error("eventfd");
	
	if(pthread_spin_init(&th->qlock,0)!=0) handle_error("pthread_spin_init");

	th->active = 1;
	th->result_cb = result_cb;
	s = pthread_create(&th->pth,NULL,do_task,(void*)th);
	if(s != 0) handle_error("pthread_create");
	
	return th;
}
void thread_destroy(thnode_t* th)
{	
	/* wait until task is complete */
	pthread_spin_destroy(&th->qlock);
	close(th->efd);	
	queue_free(th->q);
	free(th);
}
thpool_t* thread_pool_alloc(int num,void(*result_cb)(void*arg) )
{
	thpool_t *thp;
	int i;
	thp = malloc( sizeof(thpool_t ));
	if(!thp)return NULL;
	
	thp->thread_num = num;
	
	thp->threads = malloc( sizeof( thnode_t) * num );
	
	if(!thp->threads)return NULL;
	for(i = 0;i<num ; i++) thp->threads[i] = thread_create(result_cb);
	return thp;
}
/* 
 * rough way to find a thread with lighest load 
 *  based on number of assigned task
 */
void thread_pool_schedule_task(thpool_t* thp, void*(*task)(void*), void* arg)
{
	int i;
	int idx=-1;
	int sz;
	int tmp;
	for(i = 0 ;i<thp->thread_num ; i++){
		if(idx == -1|| sz > (tmp = queue_size( thp->threads[i]->q) )){
			idx = i;
			sz = tmp;
		}
	}
	thread_pool_assign_task(thp,task,arg,idx);
}

void thread_pool_assign_task(thpool_t* thp, void*(*task)(void*), void* arg,int no)
{
	thnode_t* th = (thnode_t*)thp->threads[no];
	uint64_t u;
    ssize_t s;
	qelement_t* qe = malloc( sizeof(qelement_t));
	
	if(!qe)	handle_error("malloc qelement_t");
	qe->task = task;
	qe->arg = arg;
	
	pthread_spin_lock(&th->qlock);
		queue_push( th->q,(void*)qe); 
	pthread_spin_unlock(&th->qlock);

	u = 1;
    s = write(th->efd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) handle_error("write");
	printf("add task to %d\n",no);	
}

void thread_pool_destroy(thpool_t* thp)
{
	int i;
	/* send kill task to all threads  */
	for(i= 0;i<thp->thread_num ; i++){
		thread_pool_assign_task(thp,kill_task,(void*)thp->threads[i],i);
	}
	for(i = 0;i<thp->thread_num ; i++){
		pthread_join( thp->threads[i]->pth,NULL);
		thread_destroy(thp->threads[i]);
	}
	free(thp->threads);
	free(thp);
} 
