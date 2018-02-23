createimage.c: 用于生成镜像文件image
bootloader.c：加载内核用的boot
kernel.c：内核
lock.c: 锁的实现
scheduler.c: 调度器的实现，以及do_yeild,do_exit,block,unblock
entry_mips.S: 保存山下文的save_pcb(),用来调用scheduler()并恢复上下文的scheduler_entry()
queue.c: 队列操作的实现
syslib.c: 系统调用函数。进程的yeild需要先用系统调用调do_yeild()
util.c：一些打印信息、获取时间用的函数
th1.c~th4.c,process1.c~process3.c:测试用的各种task
task.c：定义了每次实验要用哪些task
各个.h：每个与.h名字相同的.c文件所需要的一些函数、宏、变量的定义








