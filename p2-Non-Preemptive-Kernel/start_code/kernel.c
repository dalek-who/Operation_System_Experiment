/*
   kernel.c
   the start of kernel
   */

#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "th.h"
#include "util.h"
#include "queue.h"

#include "tasks.c"

volatile pcb_t *current_running;

queue_t ready_queue, blocked_queue;
struct queue R_Q, B_Q;
pcb_t *ready_arr[NUM_TASKS];
pcb_t *blocked_arr[NUM_TASKS];

pcb_t pcb[NUM_TASKS];

/*
   this function is the entry point for the kernel
   It must be the first function in the file
   */

#define PORT3f8 0xbfe48000

 void printnum(unsigned long long n)
 {
   int i,j;
   unsigned char a[40];
   unsigned long port = PORT3f8;
   i=10000;
   while(i--);

   i = 0;
   do {
   a[i] = n % 16;
   n = n / 16;
   i++;
   }while(n);

  for (j=i-1;j>=0;j--) {
   if (a[j]>=10) {
      *(unsigned char*)port = 'a' + a[j] - 10;
    }else{
	*(unsigned char*)port = '0' + a[j];
   }
  }
  printstr("\r\n");
}

void _stat(void){

	/* some scheduler queue initialize */
	/* need student add */
    printstr("debug before init\n");

	/* Initialize the PCBs and the ready queue */
	/* need student add */
	//init block and ready queue
    ready_queue = &R_Q;
	queue_init(ready_queue);
	ready_queue->pcbs = ready_arr;
	ready_queue->capacity = NUM_TASKS;

    blocked_queue = &B_Q;
	queue_init(blocked_queue);
	blocked_queue->pcbs = blocked_arr;
	blocked_queue->capacity = NUM_TASKS;


	//init all pcb ,and pop into ready_queue
	int i;
	//pcb_t *pcb;
	for(i=0 ; i<NUM_TASKS ; ++i){
        //init
        //pcb = (pcb_t*)malloc(sizeof(pcb_t));
		pcb[i].pid = i;
        //resigters:
		pcb[i].reg_s0 = 0;
		pcb[i].reg_s1 = 0;
		pcb[i].reg_s2 = 0;
		pcb[i].reg_s3 = 0;
		pcb[i].reg_s4 = 0;
		pcb[i].reg_s5 = 0;
		pcb[i].reg_s6 = 0;
		pcb[i].reg_s7 = 0;

        //todo
/*
		pcb[i].reg_k0 = 0;
		pcb[i].reg_k1 = 0;
*/	pcb[i].reg_sp = STACK_MAX - i*STACK_SIZE; // stack start from high address.
		pcb[i].reg_ra = task[i]->entry_point;
        
		printstr("debug pcb:");printnum(pcb[i].pid);
		printstr("    entry:\n");printnum(pcb[i].reg_ra);
                //push
		queue_push(ready_queue,&pcb[i]);

	}
	
    printstr("debug before clean screen\n");
	clear_screen(0, 0, 30, 24);

	/*Schedule the first task */
	scheduler_count = 0;
	scheduler_entry();

	/*We shouldn't ever get here */
	ASSERT(0);
}
