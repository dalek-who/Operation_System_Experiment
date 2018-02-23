/* Author(s): <Your name here>
 * COS 318, Fall 2013: Project 3 Pre-emptive Scheduler
 * Implementation of the process scheduler for the kernel.
 */

#include "common.h"
#include "interrupt.h"
#include "queue.h"
#include "printf.h"
#include "scheduler.h"
#include "util.h"
#include "syslib.h"

pcb_t *current_running;
node_t ready_queue;
node_t sleep_wait_queue;
// more variables...
volatile uint64_t time_elapsed;
int sleep_number = 0;
node_t temp_queue;
int have_initialed=0;

int debug_number=1;

void debug(){
    /*
    static int i=0;
    char c[10];
    itoa(i,c);
    printstr(c);
    printstr("\n");
    ++i;    
    */
    char c[10];
    itoa(debug_number,c);
    printstr(c);
    printstr("\n");
}

/* TODO:wake up sleeping processes whose deadlines have passed */
void check_sleeping(){
    pcb_t* pcb;

  //  printstr("wang check sleep()\n");
    int task_num_in_sleep_queue = sleep_number;
    int i;
    for(i=0 ; i<task_num_in_sleep_queue ; ++i){
        pcb = (pcb_t*)dequeue(&sleep_wait_queue);
        //todo
        if(time_elapsed*1000 >= pcb->deadline){
            pcb->status = READY;
            enqueue(&ready_queue,(node_t*)pcb);
            --sleep_number;
        }
        else{
            enqueue(&sleep_wait_queue,(node_t*)pcb);
        }
    }
}

/* Round-robin scheduling: Save current_running before preempting */
void put_current_running(){
    current_running->status= READY;
    enqueue(&ready_queue,(node_t*)current_running);
}


/* Change current_running to the next task */
void scheduler(){
     ASSERT(disable_count);
 //    printstr("into scheduler()\n");
     check_sleeping(); // wake up sleeping processes
     while (is_empty(&ready_queue)){
          leave_critical();
          enter_critical();
          check_sleeping();
     }
     current_running = (pcb_t *) dequeue(&ready_queue);
     ASSERT(NULL != current_running);
     ++current_running->entry_count;
//     reset_nested_count();
//     if(current_running == NULL) printstr("error:current NULL\n");
//     char addr[16];
//     itohex(current_running->entry_point,addr);
//     printstr(addr);
//     printstr("\noff scheduler()\n");
}

int lte_deadline(node_t *a, node_t *b) {
     pcb_t *x = (pcb_t *)a;
     pcb_t *y = (pcb_t *)b;

     if (x->deadline <= y->deadline) {
          return 1;
     } else {
          return 0;
     }
}

void do_sleep(int milliseconds){
     ASSERT(!disable_count);

     enter_critical();
     // TODO
        current_running->status = SLEEPING;
        ++sleep_number;
        current_running->deadline = get_timer()*1000 + milliseconds; 
        enqueue(&sleep_wait_queue,(node_t*)current_running);
        scheduler_entry();
}

void do_yield(){    
     enter_critical();
     put_current_running();
     scheduler_entry();
}

void do_exit(){
     enter_critical();
     current_running->status = EXITED;
     scheduler_entry();
     /* No need for leave_critical() since scheduler_entry() never returns */
}

void block(node_t * wait_queue){
     ASSERT(disable_count);
     current_running->status = BLOCKED;
     enqueue(wait_queue, (node_t *) current_running);
     scheduler_entry();
     enter_critical();
}

void unblock(pcb_t * task){
     ASSERT(disable_count);
     task->status = READY;
     enqueue(&ready_queue, (node_t *) task);
}

pid_t do_getpid(){
     pid_t pid;
     enter_critical();
     pid = current_running->pid;
     leave_critical();
     return pid;
}

uint64_t do_gettimeofday(void){
     return time_elapsed;
}

priority_t do_getpriority(){
	/* TODO */
}


void do_setpriority(priority_t priority){
	/* TODO */
}

uint64_t get_timer(void) {
     return do_gettimeofday();
}
