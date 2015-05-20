
#include "thread_pool.h"
int main()
{
	thpool_t * thp;
	thp = thread_pool_alloc(2);
	
	thread_pool_destroy();
	return 0;
}
