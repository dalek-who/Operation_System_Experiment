#include "common.h"
#include "syslib.h"
#include "util.h"
#include "scheduler.h"

//defined in th3.c,
uint64_t process_time;


void _start(void)
{
	/* need student add */
	// do_yield from thread5 in th3.c
	yield();
    process_time = get_timer();
    yield();
    process_time = get_timer() - process_time;
	printstr("thread 5 to process 3: ");
	printnum2(process_time);
	exit();

}
