# thread-pool

1. pthread based 
2. users can set size of thread pool
3. each thread maintains its own work queue. When there is no task, 
   thread goes to sleep.
4. Users can dispatch task to specified thread or schedule task to
   thread with the least pending tasks.

