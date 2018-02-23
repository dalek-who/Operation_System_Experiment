#include "common.h"
#include "scheduler.h"
#include "util.h"

uint64_t time;

void thread4(void)
{
	time = get_timer();
	do_yield();
	do_exit();
}

void thread5(void)
{
	//do_yield from thread4
	time = get_timer() - time;
	printstr("thread4 to thread 5: ");
	printnum(time);

//	time = get_timer();
//	do_yield();
	do_exit();

}
