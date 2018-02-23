1.没有使用sd卡，用一个文件作为虚拟sd卡来做的。common.h的宏
#define DISK_PATH    "/home/wang/Documents/OS-exp/project6-start-code-wang/vdisk"
是这个文件的绝对地址。可以改成别的。
touch vdisk，之后才能运行

2.task1全部完成（superblock、目录、文件操作）。bonus1的函数都做了。并且都自测通过。

3.read和write用echo和cat可以运行，但是vim可能会出问题

4.在我的文件系统下执行以下命令：
echo    \#include\<stdio.h\>                    >> hello.c
echo    int main\(\)\{                          >> hello.c
echo        printf\(\"hello world! \\n\"\)\;    >> hello.c
echo        return 0\;                          >> hello.c
echo    \}                                      >> hello.c
可以创建一个hello world程序，编译的过程也需要读写很多文件，能正确编译出来并执行，说明我的读写还是靠谱的。

5.我测试时这些指令都是能执行的，如果有哪个指令无法执行请联系我，我会给出一个能运行的case

6.有时候可能运行有点慢，多等一会

7.fio测了，因为速度太慢没有测完，但至少写的过程没fio报错。
