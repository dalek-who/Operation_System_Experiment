#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NUM 1000
int main(int argc, char const *argv[])
{
    FILE *file = fopen("myrand.h","wb+");
    srand((unsigned)time(NULL));
    int first=1;
    fprintf(file, "#ifndef MYRAND_H\n#define MYRAND_H\n\n\n");
    fprintf(file, "int myrand_array[]={\n");

    int i;
    for(i=0;i<MAX_NUM;++i){
        fprintf(file, "%c%d\n",(first)?' ':',' , rand());
        first = 0;
    }
    fprintf(file, "};");
    fprintf(file, "\n\n\n\n#endif\n");
    return 0;
}