/* scheduler.c */

#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "util.h"
#include "queue.h"

int scheduler_count;
// process or thread runs time
uint64_t cpu_time;

void printstr(char *s);
void printnum(unsigned long long n);
void scheduler(void)
{
	++scheduler_count;

	// pop new pcb off ready queue
	/* need student add */
	current_running = queue_pop(ready_queue);
	//empty queue would pop NULL, and NULL doesn't have state
	if(current_running != NULL){
		current_running ->state = PROCESS_RUNNING;
	}

}

void do_yield(void)
{
	save_pcb();
	/* push the qurrently running process on ready queue */
	/* need student add */
	queue_push(ready_queue,current_running);
	current_running -> state = PROCESS_READY;

	// call scheduler_entry to start next task
	scheduler_entry();

	// should never reach here
	ASSERT(0);
}

void do_exit(void)
{
	/* need student add */
	//when task is scheduled, it has already been poped.
	//exit means we no longer need this task, so we don't pop or push it, only change its state
	current_running -> state = PROCESS_EXITED;

	scheduler_entry();
}

void block(void)
{
	save_pcb();
	/* need student add */
	queue_push(blocked_queue, current_running);
	current_running -> state = PROCESS_BLOCKED;
	scheduler_entry();

	// should never reach here
	ASSERT(0);
}

//todo
int unblock(void)
{
    pcb_t* pcb;
	/* need student add */
    while(!blocked_queue->isEmpty){
        pcb = queue_pop(blocked_queue);
        queue_push(ready_queue,pcb);
    }
}

bool_t blocked_tasks(void)
{
	return !blocked_queue->isEmpty;
}
