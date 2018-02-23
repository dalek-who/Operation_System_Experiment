#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void t1(){
	char short_text[]="hello world!\n";
	char long_text[]= "/home/wang/Documents/OS-exp/project6-start-code-wang/mnt/f1";

	char short_buf[sizeof(short_text)];
	char long_buf[sizeof(long_text)];

	char use[3];
	char long_or_short[10];
	for( ; ;){
		scanf("%s",use);
		scanf("%s",long_or_short);
		FILE* fp;
		fp=fopen("/home/wang/Documents/OS-exp/project6-start-code-wang/mnt/f1",use);
		if( strcmp(use,"w")==0 || strcmp(use,"a")==0){
			if(strcmp(long_or_short,"short")==0)
				fwrite(short_text,sizeof(short_text),1,fp);
			if(strcmp(long_or_short,"long")==0)
				fwrite(long_text,sizeof(long_text),1,fp);
		}
		if( strcmp(use,"r")==0 ){
			fread(long_buf,10*sizeof(short_text),1,fp);
			printf("%s\n",long_buf);
		}
		fclose(fp);
	}

}

void t2(){
	char buf[4096];
	int i;
	FILE* fp;
	fp=fopen("/home/wang/Documents/OS-exp/project6-start-code-wang/mnt/f3","a+");
	for(i=0; i<1024+2+10 ; ++i){
//		fseek(fp,i*4096-1,SEEK_SET);
		memset(buf,'a'+ i%30,4096);
		fwrite(buf,sizeof(buf),1,fp);
		fflush(fp);
		printf("%d\n",i);
	}
	fclose(fp);

}

void t3(){
	int size1 = 2   * 4096 * sizeof(char);
	int size2 = 256 * 4096 * sizeof(char);
	int size3 = 10  * 4096 * sizeof(char);

	char* buf1=(char*)malloc(size1);
	char* buf2=(char*)malloc(size2);
	char* buf3=(char*)malloc(size3);

	memset(buf1,'a',size1);
	memset(buf2,'b',size2);
	memset(buf3,'c',size3);

	int i;
	FILE* fp;
	fp=fopen("/home/wang/Documents/OS-exp/project6-start-code-wang/mnt/f3","a+");

	fwrite(buf1,size1,1,fp);
	fflush(fp);
	printf("dir\n");

	for(i=0; i<4 ; ++i){
		fwrite(buf2,size2,1,fp);
		fflush(fp);
		printf("%d\n",i);
	}

	fwrite(buf3,size3,1,fp);
	fflush(fp);
	printf("second\n");

	fclose(fp);
	free(buf1);
	free(buf2);
	free(buf3);

	return;

}

int main(){
	t3();
	return 0;
}
