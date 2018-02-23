#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Elf32_Phdr * read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr)
{
}

void write_bootblock(FILE **imagefile, FILE *boot_file, Elf32_Ehdr *boot_header,
		Elf32_Phdr *boot_phdr)
{

}

void write_kernel(FILE **image_file, FILE *kernel_file, Elf32_Ehdr *kernel_ehdr, Elf32_Phdr *kernel_phdr)
{
}

int count_kernel_sectors(Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr){
}

void record_kernel_sectors(FILE **image_file, Elf32_Ehdr *kernel_ehdr, Elf32_Phdr *kernel_phdr, int num_sec){

}

void extended_opt(Elf32_Phdr *boot_phdr, int k_phnum, Elf32_Phdr *kernel_phdr, int num_sec){
}


int main(int argc, char *argv[]){
}
