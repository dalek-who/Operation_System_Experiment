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
#include "myrand.h"

pcb_t *current_running;
node_t ready_queue;
node_t sleep_wait_queue;
// more variables...
volatile uint64_t time_elapsed;
int sleep_number = 0;
node_t temp_queue;
int have_initialed=0;

int debug_number=1;
int print_row=5;
int schedule_time = 0;

extern int weight_table[];
extern int scheduler_table[];
extern pcb_t pcb[];
extern int num_tasks;

void debug(){
    char c[10];
    itoa(debug_number,c);
    printstr(c);
    printstr("\n");
}

int myrand(){
    static int i = 0;
    int random = myrand_array[i];
    i = (i+1)%1000;
    return random;
}


/* TODO:wake up sleeping processes whose deadlines have passed */
void check_sleeping(){
  /* pcb_t* pcb;

    printf(print_row++,50,"wang check sleep()\n");
*/
 /*   int task_num_in_sleep_queue = sleep_number;
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
    */
/*
    int i;
    pcb_t* p;
    for(i=0; i<num_tasks ; ++i){
        if(time_elapsed*1000 >= pcb[i].deadline && pcb[i].status == SLEEPING){
            pcb[i].status = READY;
            p=(pcb_t*)dequeue(&sleep_wait_queue);
            enqueue(&ready_queue,(node_t*)p);
            --sleep_number;
        }
        else
            continue;
    }
*/
}

/* Round-robin scheduling: Save current_running before preempting */
void put_current_running(){
    current_running->status= READY;
    enqueue(&ready_queue,(node_t*)current_running);
}

/* Change current_running to the next task */
void scheduler(){
     ASSERT(disable_count);
     printf(5,50,"into scheduler()\n");
  /*   check_sleeping(); // wake up sleeping processes
     while (is_empty(&ready_queue)){
          leave_critical();
          enter_critical();
          check_sleeping();
     }
     */
//     current_running = (pcb_t *) dequeue(&ready_queue);
  
  //  printf(print_row++,50,"after check sleeping\n");

    //weight based scheduler
     int total_weight = scheduler_table[num_tasks-1];
     int my_rand = myrand();
     int random = my_rand % total_weight;
     int could_sheduler = 0;
     int new_task;

  //   printf(6, 50, "                                               ");
     printf(6, 50, "my_rand: %d total weight: %d random : %d     \n",my_rand,total_weight,random);

     for(new_task=0; new_task < num_tasks ; ++new_task){
        if( scheduler_table[new_task] <= random)
            continue;
        else{
            could_sheduler = (pcb[new_task].status == READY || pcb[new_task].status == FIRST_TIME)?1:0;
            break;
        }
     }

     printf(7, 50,"after first random\n");

     if(!could_sheduler){
        for( ;  ; new_task = (new_task+1)%total_weight){
            could_sheduler = (pcb[new_task].status == READY || pcb[new_task].status == FIRST_TIME)?1:0;
            if(could_sheduler)
                break;
            else
                continue;
        }
     }

     printf(8, 50,"find new task\n");

     dequeue(&ready_queue);
     current_running = &pcb[new_task];

     printf(9, 50,"finish schedule\n");
     printf(10, 50,"choose task %d,address 0x%x\n",new_task,current_running->entry_point );
     ASSERT(NULL != current_running);
     ++current_running->entry_count;

//     print_status();

     printf(11, 50,"leave scheduler()\n");
     printf(12, 50,"%d 'th schedule\n",++schedule_time);

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
