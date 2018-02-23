#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//int buf[512]={0};

int count_sectors(Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr,int *size){
	int sectors_num;
	int more;
	//todo
    int i;
    for(i=0, *size=0 ; i<kernel_header->e_phnum ; ++i ){
        *size += kernel_phdr[i].p_filesz;
    }
	sectors_num = *size / 512; //512K,the size of one sector
	more = *size % 512;
	if(more>0)
		sectors_num = sectors_num + 1;
	return sectors_num;
	
}

Elf32_Phdr * read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr)
{
	//为ehdr分配空间
    *ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
    if( ehdr== NULL ){
        printf("malloc %s ehdr error\n",filename);
        exit(0);
    }
    
    //打开可执行文件
	*execfile = fopen(filename,"rb"); //open elf file

    if(*execfile == NULL){
        printf("no file %s\n",filename);
        return NULL;
    }
    
    //文件指针不是这么用的
//	*ehdr =(Elf32_Ehdr*) *execfile; //elf head is at the top of elf file,so elf head's addr is file's addr
//	phdr  =(Elf32_Phdr*) *execfile + (*ehdr)->e_phoff;
    
    //读出位于elf文件最前面的Ehdr
    fread(*ehdr,sizeof(Elf32_Ehdr),1,*execfile);
    
    //读Phdr
    //定位到第一个Phdr的位置
    fseek(*execfile,(*ehdr)->e_phoff,SEEK_SET);
    //给phdr分配空间
    //每个elf文件的ehdr只有一个，但是phdr可以有很多（ehdr->e_phnum个），这些Phdr都要读出来。
    //各个Phdr是挨在一起的，size也一样大，所以可以用数组的方式调用各个phdr
    Elf32_Phdr * phdr=(Elf32_Phdr*)malloc( ((*ehdr)->e_phnum) * sizeof(Elf32_Phdr));
    if( phdr== NULL ){
        printf("malloc %s phdr error\n",filename);
        exit(0);
    }
    fread(phdr,sizeof(Elf32_Phdr),(*ehdr)->e_phnum,*execfile);
	return phdr;
}

void write_bootblock(FILE **imagefile, FILE *boot_file, Elf32_Ehdr *boot_header, Elf32_Phdr *boot_phdr)
{
	//FILE * image_start = *imagefile;
    char buf[512] = "";
	fseek(boot_file,boot_phdr->p_offset,SEEK_SET);
	fread(buf,boot_phdr->p_filesz,1,boot_file);
    fwrite(buf,1,512,*imagefile);
	//memset(*imagefile,0,512*1024 - boot_phdr->p_filesz); //clear rest parts of the the first sector
	//todo
	//*imagefile = image_start + 512; //put *image at the first byte of the 2ed sector
	return;
}

void write_kernel(FILE **image_file, FILE *kernel_file, Elf32_Ehdr *kernel_ehdr, Elf32_Phdr *kernel_phdr)
{   
    //计算出kernel所需的size以及最终所需要填满的扇区数后，在内存中开出相应大小的buffer
    int size;
    int sector_num = count_sectors(kernel_ehdr,kernel_phdr,&size);
    char buf[0x10000]="";
    //把buffer先清为0，这样在把程序读进buffer后，再把buffer读进ssd后，扇区没用完的部分剩下的就是用0填满了
    
    //分多次把各个phdr对应的可执行代码读进buffer，然后一次把buffer内容写进image
    int i,already_read;
    //挨个把每个phdr下面对应的可执行代码读进buffer
    for(i=0,already_read=0; i<kernel_ehdr -> e_phnum ; ++i){
        fseek(kernel_file,kernel_phdr[i].p_offset,SEEK_SET);
        fread(buf+already_read,kernel_phdr[i].p_filesz,1,kernel_file);
        already_read += kernel_phdr[i].p_filesz;
    }
    //把装有可执行代码的buffer内容写进image
    fwrite(buf,1,0x10000-512,*image_file);
    
    //把三个process写进内核。如果少于三个，文件指针会返回NULL
    char process_name[3][15] = {"process1","process2","process3"};
    FILE *p_process[3];
    Elf32_Ehdr* ehdr_process[3];
    Elf32_Phdr* phdr_process[3];
    int j=0;
    for(i=0;i<3;++i){
        memset(buf,0,0x10000);
        phdr_process[i] = read_exec_file(&p_process[i], process_name[i], &ehdr_process[i]);
        if(p_process[i] == NULL){
            printf("********************no process %d\n",i+1);
            continue;
        } 
        for(j=0,already_read =0; j<ehdr_process[i]->e_phnum ; ++j){
            fseek(p_process[i],phdr_process[i][j].p_offset,SEEK_SET);
            fread(buf+already_read,phdr_process[i][j].p_filesz,1,p_process[i]);
            already_read += phdr_process[i][j].p_filesz;
        }
        fwrite(buf,1,0x10000,*image_file);
        printf("**********************write process %d\n",i+1);
        fclose(p_process[i]);
    }
    
	return;
}


void record_kernel_sectors(FILE **image_file, Elf32_Ehdr *kernel_ehdr, Elf32_Phdr *kernel_phdr, int num_sec){
    int size[1]={2};
    size[0]=num_sec*512;
	//定位到bootloader中size变量的地址
//	fseek(*image_file,0x20,SEEK_SET);
    //写进新的机器码，覆盖原有机器码
//	fwrite(size,1,4,*image_file);
    
    /*
    //实现思路1：
    //这个imm初始化的值后四位是立即数，代表kernel的大小（目前是全0）。前面的位是操作码以及要load的寄存器号
    int imm[1]= {0x24060000};
    //后四位的立即数改为kernel的大小
	imm[0] += (num_sec*512);
    //定位到那条机器码所在的位置（它相对于文件起始位置的相对位置，是通过objdump找到的）
	fseek(*image_file,0x40,SEEK_SET);
    //写进新的机器码，覆盖原有机器码
	fwrite(imm,1,4,*image_file);
    */
    
    /*//实现思路2：
    //注：我们用的都是冯诺依曼结构的计算机，数据和指令都用同一个ram，所以才能做到用数据覆盖原本是指令的内存空间
    //算出kernel大小
    int size[1];
    size[0] = num_sec*512;
    //定位到main的第一个nop地址（即image的开头地址）
    fseek(*image_file,0,SEEK_SET);
    //在内存中写入kernel的地址，覆盖掉第一个nop
    fwrite(imm,1,4,*image_file);
    */
	return;
}

void extended_opt(Elf32_Phdr *boot_phdr, int k_phnum, Elf32_Phdr *kernel_phdr, int num_sec,int kernel_size){
	printf("bootblock:\n");
	printf("sector: 1\n");
	printf("write byte on SSD: 512\n");
	printf("file size:%d\n",boot_phdr->p_filesz);
    printf("offset: %d\n",boot_phdr->p_offset);

	printf("kernel:\n");
	printf("section: %d\n",num_sec);
	printf("write byte on SSD: %d\n",num_sec*512);
	printf("file size:%d\n",kernel_size);
    printf("offset: %d\n",kernel_phdr->p_offset);
    
	return;
	
}


int main(int argc, char *argv[]){
	FILE *image = fopen("image","wb+");
    int extended=0;
	if(image == NULL) {
		printf("error open image\n");
		exit(0);
	}
	FILE *boot_file, *kernel_file;

	Elf32_Ehdr *boot_ehdr,*kernel_ehdr;
	Elf32_Phdr *boot_phdr,*kernel_phdr;
    
    //确定是否有--extended扩展功能
    int i;
    for(i=0; i<argc ; ++i){
		if (strcmp(argv[i],"--extended")==0)
                extended = 1;
	}
	
    //打开bootblock和kernel两个文件，并读出各自的phdr、ehdr
    boot_phdr = read_exec_file(&boot_file, "bootblock", &boot_ehdr);
	kernel_phdr = read_exec_file(&kernel_file, "kernel", &kernel_ehdr);
	
    //把kernel和bootblock的可执行代码写进image
    write_bootblock(&image, boot_file,   boot_ehdr,   boot_phdr);
	write_kernel   (&image, kernel_file, kernel_ehdr, kernel_phdr);

	int size;
	int num_sec = count_sectors(kernel_ehdr,kernel_phdr,&size);
    //改变bootloader的.data段的变量size的值，改变要load的kernel的大小
	record_kernel_sectors(&image,kernel_ehdr, kernel_phdr, 0x40000/512);
	
    
    if(extended)
        extended_opt(boot_phdr,kernel_ehdr->e_phnum, kernel_phdr, num_sec, size);
	
    fclose(image);
	fclose(boot_file);
	fclose(kernel_file);
}
