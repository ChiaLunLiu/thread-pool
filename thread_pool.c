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
	void (*task)(void* arg); /* task function should free arg */
	void* arg;
}qelement_t;

static void qelement_free(void* ptr)
{
		qelement_t * q = (qelement_t*)ptr;
		free(q);
}
/*
 *  after kill_task is called, we should not insert any task to queue
 */
void kill_task(void* arg)
{
	thnode_t* th = (thnode_t*)arg;
	/* pool would free the thread resource */
	th->active = 0;
}


static void* do_task(void* arg)
{
	    int s;
	    uint64_t u;
		thnode_t* th = (thnode_t*)arg;		
		qelement_t * qe;
		while(th->active){
			s = read(efd, &u, sizeof(uint64_t));
            if (s != sizeof(uint64_t)) handle_error("read");
            
            for(i = 0; th->active && i< s ; i++){
				pthread_spin_lock(&th->qlock);
					qe = (qelement_t*) queue_pop( th->q); 
				pthread_spin_unlock(&th->qlock);
					if(!qe){
							/* this shouldn't happen */
							handle_error("queue is null");
					}
				qe->task(qe->arg);
				qelement_free(qe);
			}
		}
		
		return NULL;
}
thnode_t* thread_create()
{
	int s;
	thnode_t* th;
	th = malloc( sizeof( thnode_t ));
	if(!th) handle_error("malloc thnode_t");
	
	th->q = queue_alloc(qelement_free);
	
	if(!th->q)handle_error("malloc queue_t");
	
	th-->efd = eventfd(0,0);
	if(th->efd == -1) handle_error("eventfd");
	
	if(pthread_spin_init(&th->qlock,0)!=0) handle_error("pthread_spin_init");

	s = pthread_create(&th->pth,NULL,do_task,(void*)th);
	if(s != 0) handle_error("pthread_create");
	
	return th;
}
void thread_destroy(thnode* th)
{	
	/* wait until task is complete */
	pthread_spin_destroy(&th->qlock);
	fclose(th->efd);
	queue_free(th->q);
	free(th);
}
int thread_pending_jobs(thnode* th)
{
	return queue_size( th->q);
}
thpool_t* thread_pool_alloc(int num)
{
	thpool_t *thp;
	int i;
	thp = malloc( sizeof(thpool_t ));
	if(!thp)return NULL;
	
	thp->thread_num = num;
	
	thp->threads = malloc( sizeof( thnode_t) * num );
	
	if(!thp->threads)return NULL;
	for(i = 0;i<num ; i++) thp->threads[i] = thread_create();
	return thp;
}
/* 
 * rough way to find a thread with lighest load 
 *  based on number of assigned task
 */
void thread_pool_schedule_task(thpool_t* thp, void(*)(void*), void* arg)
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
	thread_pool_assign_task(thp,task,arg,idx);
}
void thread_pool_assign_task(thpool_t* thp, void(*task)(void*), void* arg,int no)
{
	thnode_t* th = (thnode_t*)arg;
	uint64_t u;
    ssize_t s;
	qelement_t* q = malloc( sizeof(qelement_t));
	
	if(!q)	handle_error("malloc qelement_t");

	q->task = task;
	q->arg = arg;
	
	pthread_spin_lock(&th->qlock);
		queue_push( th->q[no],qe); 
	pthread_spin_unlock(&th->qlock);
	
	u = 1;
    s = write(efd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) handle_error("write");
}
void thread_pool_destory(thpool_t* thp,)
{
	int i;
	/* send kill task to all threads */
	for(i= 0;i<thp->thread_num ; i++){
		thread_pool_assign_task(thp,kill_task,(void*)thp->threads[i],i);
	}
	for(i = 0;i<thp->thread_num ; i++){
		pthread_join( thp->threads[i]->pth,NULL);
		thread_destroy(thp->threads[i]->pth);
	}
}
