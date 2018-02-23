#include "common.h"

/*define global variables here*/

/*
 Use linear table or other data structures as you need.
 
 example:
 struct file_info[MAX_OPEN_FILE] fd_table;
 struct inode[MAX_INODE] inode_table;
 unsigned long long block_bit_map[];
 Your dentry index structure, and so on.
 
 
 //keep your root dentry and/or root data block
 //do path parse from your filesystem ROOT<@mount point>
 
 struct dentry* root;
 */

#define DEBUG_CHECK_DIRECTORY 8

#define UNUSED 0
#define USED   1

#define FIND 1
#define NOT_FIND 0

//one bitmap == 1 block == 4KB == 32K bit == 1K int

superblock_t superblock;
superblock_t backup_superblock;
/*
char     inode_bitmap[INODE_BITMAP_BLOCKS_NUM * BLOCK_SIZE];
char datablock_bitmap[ DATA_BITMAP_BLOCKS_NUM * BLOCK_SIZE];
inode_disk_t   inode_disk_table[INODE_NUM];
inode_mem_t    inode_mem_table[INODE_NUM];
file_info file_info_table[MAX_OPEN_FILE];
*/
/*
char*     inode_bitmap;
char* datablock_bitmap;
inode_disk_t*   inode_disk_table;
inode_mem_t*    inode_mem_table;
file_info* file_info_table;
*/

char inode_bitmap[INODE_BITMAP_BLOCKS_NUM * BLOCK_SIZE]     ;
char datablock_bitmap[DATA_BITMAP_BLOCKS_NUM * BLOCK_SIZE]  ;
inode_disk_t   inode_disk_table[INODE_NUM] ;
inode_mem_t    inode_mem_table[INODE_NUM]  ;
file_info      file_info_table[MAX_OPEN_FILE]  ;

directory_t*      debug_directory[DEBUG_CHECK_DIRECTORY];
indirect_block_t* debug_indirect_block[DEBUG_CHECK_DIRECTORY];

typedef enum {
    second_last_find___last_not_find, //例：/home/wang/os/hw ，os找到，hw未找到
    second_last_find___last_find,     //例：/home/wang/os/hw ，os找到，hw找到
    mid_not_find                      //例：/home/wang/os/hw ，wang未找到
} path_analizing_result ;

//两个debug函数：从sd卡里抓出前i个inode的文件（默认以目录文件形式读出）
void init_debug_directory(){
    int i;
    for(i=0;i<1;++i){
        debug_directory[i] = (directory_t*)malloc(BLOCK_SIZE);
        if(debug_directory[i] == NULL)
        	return 0;
    }
}

void read_debug_directory(){
    int i;
    for(i=0; i<DEBUG_CHECK_DIRECTORY;++i){
        read_file(debug_directory[i],i);
    }
}

//两个debug函数：从sd卡里抓出前i个间接块
void init_debug_indirect_block(){
    int i;
    for(i=0;i<DEBUG_CHECK_DIRECTORY;++i){
        debug_indirect_block[i] = (indirect_block_t*)malloc(BLOCK_SIZE);
        if(debug_indirect_block[i] == NULL)
        	return 0;
    }
}

void read_debug_indirect_block(){
    int i;
    for(i=0;i<DEBUG_CHECK_DIRECTORY;++i){
    	device_read_sector(debug_indirect_block[i],DATA_BLOCKS_LOC+i);
    }
}

//向上取整的除法
int ceil_division(int a,int b){
    //公式： ceil(a÷b) = floor( (a-1)÷b ) +1
    return (a==0)?0:(a-1)/b + 1;
}

int inode_mem_pointer_to_index(inode_mem_t* inode_mem){
    return (inode_mem - inode_mem_table)/sizeof(inode_mem_t);
}

inode_mem_t* inode_mem_index_to_pointer(int inode_index){
    return &inode_mem_table[inode_index];
}

int inode_disk_pointer_to_index(inode_disk_t* inode_disk){
    return (inode_disk - inode_disk_table)/sizeof(inode_disk_t);
}

inode_disk_t* inode_disk_index_to_pointer(int inode_index){
    return &inode_disk_table[inode_index];
}
/*
int get_file_datablock_index_array(int datablock_index_array[],int inode_index){
    inode_disk_t* inode = &inode_disk_table[inode_index];
    int i;
    for(i=0; i<inode->blocks_number && i<DIRECT_DATA_BLOCKS_NUM ; ++i){
        datablock_index_array[i] = inode->direct_block[i];
    }

    if(inode->blocks_number > DIRECT_DATA_BLOCKS_NUM){
        indirect_block_t indirect_block;
        device_read_sector(&indirect_block,DATA_BLOCKS_LOC + inode->indirect_block);
        for(i=0; i< inode->blocks_number - DIRECT_DATA_BLOCKS_NUM ; ++i){
            datablock_index_array[DIRECT_DATA_BLOCKS_NUM +i] = indirect_block.indirect_block_table[i];
        }
    }

    return 0;
}
*/

int count_second_indirect_blocks_num(int file_size){
    int second_indirect_blocks_num = 0;
    int blocks_num = ceil_division(file_size,BLOCK_SIZE);
    if(blocks_num <= DIRECT_DATA_BLOCKS_NUM)
        return 0;
    else
        return blocks_num - DIRECT_DATA_BLOCKS_NUM;

}


int get_file_indirect_block_index_array(int second_indirect_block_index_array[],int inode_index){
    inode_disk_t *inode=&inode_disk_table[inode_index];
    int second_indirect_blocks_num = count_second_indirect_blocks_num(inode->file_size);
    indirect_block_t first_indirect_block;
    device_read_sector(&first_indirect_block,DATA_BLOCKS_LOC + inode->indirect_block);
    int i;
    for(i=0;i<second_indirect_blocks_num ; ++i){
        second_indirect_block_index_array[i] = first_indirect_block.indirect_block_table[i];
    }
    return 0;
}

int get_file_datablock_index_array(int datablock_index_array[],int inode_index){
    inode_disk_t *inode=&inode_disk_table[inode_index];

    //读取直接块
    int i=0,j,k; //i用来标识正文数据的datablock块
    for(j=0; i<inode->blocks_number && j<DIRECT_DATA_BLOCKS_NUM ; ++i,++j){
        datablock_index_array[i] = inode->direct_block[j];
    }

    //读取二级间接块
    int second_indirect_blocks_num = count_second_indirect_blocks_num(inode->file_size);
    indirect_block_t* second_indirect_block=(indirect_block_t*)malloc(second_indirect_blocks_num*sizeof(indirect_block_t));
    if(second_indirect_block == NULL)
    	return -ENOSPC;
    int second_indirect_block_index_array[second_indirect_blocks_num];
    get_file_indirect_block_index_array(second_indirect_block_index_array,inode_index);
    for(j=0; j<second_indirect_blocks_num ; ++j){
        device_read_sector(&second_indirect_block[j],DATA_BLOCKS_LOC + second_indirect_block_index_array[j]);
    }
    //从二级间接块里读取正文数据datablock块
    for(j=0; i<inode->blocks_number && j<second_indirect_blocks_num ; ++j){
        for(k=0; i<inode->blocks_number && k<BLOCK_SIZE/sizeof(int) ; ++i,++k){
            datablock_index_array[i] = second_indirect_block[j].indirect_block_table[k];
        }
    }

    free(second_indirect_block);
    return 0;
}

int device_read_multi_sector(unsigned char buffer[],int start_sector,int sector_num){
    int i;
    for(i=0 ; i<sector_num ; ++i){
        device_read_sector(buffer+i*SECTOR_SIZE,start_sector+i);
    }
    return 0;
}

int device_write_multi_sector(unsigned char buffer[],int start_sector,int sector_num){
    int i;
    for(i=0 ; i<sector_num ; ++i){
        device_write_sector(buffer+i*SECTOR_SIZE,start_sector+i);
    }
    return 0;
}

int device_clear_sector(int sector_num){
    char buf[BLOCK_SIZE]={0};
    device_write_sector(buf,sector_num);
    return 0;
}

int lookup_bitmap(char* bitmap,int bit_num){
    if(bit_num >= INODE_NUM){
        printf("look up bitmap out of range\n");
        return 0;
    }
    else{
        int char_index  = bit_num / 8;
        int char_offset = bit_num % 8;
        char mask = (char)0x01;
        mask = mask << char_offset;
        return ((mask & bitmap[char_index]) == 0)? UNUSED : USED;
    }
}

int set_bitmap(char* bitmap,int bit_num,int use_or_not){ //flag == 0:unuse, 1:use
    if(bit_num >= INODE_NUM){
        printf("set bitmap out of range\n");
        return 0;
    }
    else{
        int char_index  = bit_num / 8;
        int char_offset = bit_num % 8;
        /*
        char mask = (char)use_or_not;
        mask = mask << char_offset;
        bitmap[char_index] = bitmap[char_index] | mask; //use：| 00010000    unuse:& 11101111 
        */
        char mask = (char)0x01;
        mask = mask << char_offset;
        bitmap[char_index] = (use_or_not == USED)? bitmap[char_index]|mask : bitmap[char_index]&~mask;

        return use_or_not;
    }
}

int read_bitmap(char* bitmap,char which[]){
    int start_block;
    if( strcmp(which,"inode")==0 )
        start_block = INODE_BITMAP_BLOCKS_LOC;
    else if(strcmp(which,"datablock") == 0)
        start_block = DATA_BITMAP_BLOCKS_LOC;
    else{
        printf("error use of read_bitmap()\n");
        return 0;
    }

    device_read_multi_sector(bitmap,start_block,INODE_BITMAP_BLOCKS_NUM);   // INODE_BITMAP_BLOCKS_NUM == DATA_BITMAP_BLOCKS_NUM
};

int write_bitmap(char* bitmap,char which[]){
    int start_block;
    if( strcmp(which,"inode")==0 )
        start_block = INODE_BITMAP_BLOCKS_LOC;
    else if(strcmp(which,"datablock") == 0)
        start_block = DATA_BITMAP_BLOCKS_LOC;
    else{
        printf("error use of write_bitmap()\n");
        return 0;
    }

    device_write_multi_sector(bitmap,start_block,INODE_BITMAP_BLOCKS_NUM);   // INODE_BITMAP_BLOCKS_NUM == DATA_BITMAP_BLOCKS_NUM
};

int count_bitmap(char* bitmap){
	int used_num;
	int i;
	for(i=0 ; i<INODE_NUM ; ++i){
        used_num += lookup_bitmap(bitmap,i);
    }
    return used_num;
}

int read_inode_disk_table(){
    device_read_multi_sector(inode_disk_table,INODE_BLOCKS_LOC,INODE_BLOCKS_NUM);
    return 1;
}

int write_inode_disk_table(){
    device_write_multi_sector(inode_disk_table,INODE_BLOCKS_LOC,INODE_BLOCKS_NUM);
    return 1;
}

int file_size_to_blocks_num(int file_size){
    return ceil_division(file_size,BLOCK_SIZE);
}

int file_size_to_blocks_num_one_more(int file_size){
    int blocks_number = ceil_division(file_size,BLOCK_SIZE);
    blocks_number = (blocks_number < MAX_FILE_BLOCKS_NUM)? blocks_number+1 : blocks_number;
    return blocks_number;
}

int read_file(void* file_buf,int inode_index){

    inode_mem_t* inode_mem = inode_mem_index_to_pointer(inode_index);

    if(file_buf == NULL){
        return -ENOBUFS;
    }
        

    if(inode_mem->inode_disk->file_size >= MAX_FILE_SIZE){
        printf("read file more than max size\n");
        return -EFBIG;
    }

    int i;
/*
    for(i=0 ; i<inode_mem->inode_disk->blocks_number - inode_mem->inode_disk->file_indirect_blocks_num ; ++i){
        device_read_sector(file_buf+i*BLOCK_SIZE,DATA_BLOCKS_LOC+inode_mem->inode_disk->direct_block[i]);
    }
 
    //indirect_block_t indirect_block;
    //device_read_sector(&indirect_block,BLOCK_SIZE);
    
    if(inode_mem->inode_disk->blocks_number > DIRECT_DATA_BLOCKS_NUM){
        if(inode_mem->indirect_block == NULL){
            inode_mem->indirect_block = (indirect_block_t*)malloc(sizeof(indirect_block_t));
        }
        device_read_sector(inode_mem->indirect_block->indirect_block_table,DATA_BLOCKS_LOC+inode_mem->inode_disk->indirect_block);
    
        for(i=0 ; i<inode_mem->inode_disk->file_indirect_blocks_num ; ++i){
            device_read_sector(file_buf+(i+DIRECT_DATA_BLOCKS_NUM)*BLOCK_SIZE,DATA_BLOCKS_LOC+inode_mem->indirect_block->indirect_block_table[i]);
        }
    }
*/
    int blocks_number = inode_disk_table[inode_index].blocks_number;
    int datablock_index_array[blocks_number];
    get_file_datablock_index_array(datablock_index_array,inode_index);
    for(i=0; i<blocks_number ; ++i){
        device_read_sector(file_buf+i*BLOCK_SIZE,DATA_BLOCKS_LOC+datablock_index_array[i]);
    }
    return 0;
}

/*
int write_file(void** file_buf,int inode_index){
    
    inode_mem_t* inode_mem = inode_mem_index_to_pointer(inode_index);

    if(inode_mem->inode_disk->file_size >= MAX_FILE_SIZE){
        printf("write file more than max size\n");
        return 0;
    }

    int blocks_number = ( (inode_mem->inode_disk->file_size / (4*K) ) == MAX_FILE_BLOCKS_NUM)?  inode_mem->inode_disk->file_size / (4*K) 
                                                                                             :1+inode_mem->inode_disk->file_size / (4*K);


    int i;
    for(i=0 ; i<inode_mem->inode_disk->blocks_number - inode_mem->inode_disk->file_indirect_blocks_num ; ++i){
        device_write_sector(*file_buf+i*BLOCK_SIZE,inode_mem->inode_disk->direct_block[i]);
    }
 

    device_write_sector(inode_mem->indirect_block->indirect_block_table,inode_mem->inode_disk->indirect_block);
    for(i=0 ; i<inode_mem->inode_disk->file_indirect_blocks_num ; ++i){
        device_write_sector(*file_buf+(i+2)*BLOCK_SIZE,inode_mem->indirect_block->indirect_block_table[i]);
    }
       
    return 0;
}
*/

int write_file(void* file_buf,int inode_index){

    inode_mem_t* inode_mem = inode_mem_index_to_pointer(inode_index);

    if(file_buf == NULL){
        return -ENOBUFS;
    }

    if(inode_mem->inode_disk->file_size >= MAX_FILE_SIZE){
        printf("write file more than max size\n");
        return -E2BIG;
    }


    int i;
/*    for(i=0 ; i<inode_mem->inode_disk->blocks_number - inode_mem->inode_disk->file_indirect_blocks_num ; ++i){
        device_write_sector(file_buf+i*BLOCK_SIZE,DATA_BLOCKS_LOC+inode_mem->inode_disk->direct_block[i]);
    }
 
    //indirect_block_t indirect_block;
    //device_read_sector(&indirect_block,BLOCK_SIZE);
    if(inode_mem->inode_disk->blocks_number > DIRECT_DATA_BLOCKS_NUM){    
    	device_write_sector(inode_mem->indirect_block->indirect_block_table,DATA_BLOCKS_LOC+inode_mem->inode_disk->indirect_block);
       
        for(i=0 ; i<inode_mem->inode_disk->file_indirect_blocks_num ; ++i){
            device_write_sector(file_buf+(i+2)*BLOCK_SIZE,DATA_BLOCKS_LOC+inode_mem->indirect_block->indirect_block_table[i]);
        }
    }       
*/
    int blocks_number = inode_disk_table[inode_index].blocks_number;
    int datablock_index_array[blocks_number];
    get_file_datablock_index_array(datablock_index_array,inode_index);
    for(i=0; i<blocks_number ; ++i){
        device_write_sector(file_buf+i*BLOCK_SIZE,DATA_BLOCKS_LOC+datablock_index_array[i]);
    }

    return 0;
}

int lookup_directory(int inode_index,char file_name[]){
    if(inode_disk_table[inode_index].file_type != DIRECTORY){
        printf("path contains a non_directory\n");
        return -ENOTDIR;
    }
    directory_t *directory= (directory_t*)malloc(file_size_to_blocks_num_one_more(inode_disk_table[inode_index].file_size) * BLOCK_SIZE);
    if(directory == NULL){
        return -ENOMEM;
    }
    //read directory from disk
    read_file(directory,inode_index);
    
    int i;
    int result = NOT_FIND;
    for(i=0;i<inode_mem_table[inode_index].inode_disk->dentry_num ; ++i){
        if(strcmp(file_name,directory->dentry_array[i].file_name) == 0){
            result = FIND;
            break;
        }
        else
            continue;
    }

    int inode;
    if (result == FIND){
        inode = directory->dentry_array[i].inode;
    }        
    else{
        inode = -1;
    }

    free(directory);
    return inode;

}

//解析路径，返回查找结果.用指针buffer返回倒数第二层的inode_index，最后一层的inode_index,最后一个文件名字
path_analizing_result path_analize(char* path,char* last_file_name,int* second_last_inode_index,int* last_inode_index){
    char path_cpy1[MAX_FILE_NAME_LEN * MAX_DEEPTH]={0};
    char path_cpy2[MAX_FILE_NAME_LEN * MAX_DEEPTH]={0};
    strcpy(path_cpy1,path);
    strcpy(path_cpy2,path);

    if(strcmp(path,"/") == 0){
        *second_last_inode_index = 0;
        *last_inode_index = 0;
        strcpy(last_file_name,"/");
        return second_last_find___last_find;
        
    }


    //count max deepth
    int max_deepth=0;
    char* p=strtok(path_cpy1,"/");
    while(p != NULL){
        ++max_deepth;
        strcpy(last_file_name,p);
        p = strtok(NULL,"/");
    }

    //例子： /home/wang/os/hw
    //      0  1    2   3   4(max_deepth==4)
    int i,current_inode_index;
    for(i=0,current_inode_index=0,p=strtok(path_cpy2,"/") ; p!=NULL && i<max_deepth ; ++i){
        if(lookup_directory(current_inode_index,p) == -1 && i<max_deepth-1){
        //例：/home/wang/os/hw ，wang未找到
            *second_last_inode_index = -1;
            *last_inode_index = -1;
            return mid_not_find;
        }
        else if(lookup_directory(current_inode_index,p) == -1 && i==max_deepth-1){
        //例：/home/wang/os/hw ，os找到，hw未找到
            *second_last_inode_index = current_inode_index;
            *last_inode_index = -1;
            return second_last_find___last_not_find;
        }
        else if(lookup_directory(current_inode_index,p) != -1 && i==max_deepth-1){
        //例：/home/wang/os/hw ，os找到，hw找到
            *second_last_inode_index = current_inode_index;
            *last_inode_index = lookup_directory(current_inode_index,p);
            return second_last_find___last_find;
        }
        else{
            current_inode_index = lookup_directory(current_inode_index,p);
            p = strtok(NULL,"/");
        }
    }

}

//申请可用的bit
int apply_available_bit(char* bitmap){
    int i;
    for(i=0;i<INODE_NUM;++i){
        if(lookup_bitmap(bitmap,i) == UNUSED)
            return i;
        else
            continue;
    }

    if(i==INODE_NUM)
        return -1;
}

//在指定inode的directory里添加一个新文件
//返回新文件所在的inode
int add_file_in_directory_by_inode(int directory_inode_index,char file_name[],type file_type,mode_t mode){

    directory_t* directory=(directory_t*)malloc(file_size_to_blocks_num_one_more(inode_disk_table[directory_inode_index].file_size)*BLOCK_SIZE);
    if(directory == NULL){
        return -ENOMEM;
    }
    read_file(directory,directory_inode_index);

    //如果目录已满
    if(MAX_FILE_SIZE - inode_disk_table[directory_inode_index].file_size < sizeof(dentry)){
        free(directory);
        return -ENOSPC;
    }

    //在inode_bitmap里注册一个可用的bit
    int new_file_inode_index = apply_available_bit(inode_bitmap);
    if(new_file_inode_index == -1){
        free(directory);
        return -ENOSPC;
    }
    else{
        set_bitmap(inode_bitmap,new_file_inode_index,USED);
    }

    //在目录里添加新的目录项。由于读目录时比文件本身大小多读一个块，
    //所以只要能运行到这里，就不需要因为当前的块已满而重新realloc

    //处理当前目录
    //如果需要增加新的dabatblock：
    int new_datablock_index=-2;
    int increase = ((inode_disk_table[directory_inode_index].file_size % BLOCK_SIZE) == 0);
    if(increase){
        new_datablock_index = apply_available_bit(datablock_bitmap);
        if(new_datablock_index == -1){
            free(directory);
            return -ENOSPC;
        }
        else{
            set_bitmap(datablock_bitmap,new_datablock_index,USED);
        }
    
    }

    int file_size = inode_disk_table[directory_inode_index].file_size + sizeof(dentry);
    inode_disk_table[directory_inode_index].file_size = file_size;
    inode_disk_table[directory_inode_index].file_indirect_blocks_num = (file_size_to_blocks_num(file_size)>DIRECT_DATA_BLOCKS_NUM)? file_size_to_blocks_num(file_size)-DIRECT_DATA_BLOCKS_NUM :0;
    inode_disk_table[directory_inode_index].dentry_num += 1;
    inode_disk_table[directory_inode_index].blocks_number = file_size_to_blocks_num(file_size);
    inode_disk_table[directory_inode_index].last_modified_time = time(NULL);


    if(increase){
        //如果需要增加新的dabatblock：暂不处理
    }

    strcpy(directory->dentry_array[inode_disk_table[directory_inode_index].dentry_num-1].file_name,file_name);
    directory->dentry_array[inode_disk_table[directory_inode_index].dentry_num-1].inode = new_file_inode_index;

    //处理新的inode
    inode_disk_table[new_file_inode_index].file_size = 0;
    inode_disk_table[new_file_inode_index].file_indirect_blocks_num = 0;
    inode_disk_table[new_file_inode_index].dentry_num = 0;
    inode_disk_table[new_file_inode_index].file_type = file_type ;
    inode_disk_table[new_file_inode_index].blocks_number = 0;
    inode_disk_table[new_file_inode_index].links = 1;
    inode_disk_table[new_file_inode_index].create_time = time(NULL);
    inode_disk_table[new_file_inode_index].last_access_time = time(NULL);
    inode_disk_table[new_file_inode_index].last_modified_time = time(NULL);
    inode_disk_table[new_file_inode_index].mode = mode;

    //写入devic
    write_bitmap(inode_bitmap,"inode");
    write_bitmap(datablock_bitmap,"datablock");
    write_inode_disk_table();
    write_file(directory,directory_inode_index);
    device_flush();

    free(directory);
    return new_file_inode_index;
}

int add_dentry_in_directory_by_inode(int directory_inode_index,char file_name[],int file_inode_index){
    directory_t* directory=(directory_t*)malloc(file_size_to_blocks_num_one_more(inode_disk_table[directory_inode_index].file_size)*BLOCK_SIZE);
    if(directory == NULL){
        return -ENOMEM;
    }
    read_file(directory,directory_inode_index);

    //如果目录已满
    if(MAX_FILE_SIZE - inode_disk_table[directory_inode_index].file_size < sizeof(dentry)){
        free(directory);
        return -ENOSPC;
    }

    //在目录里添加新的目录项。由于读目录时比文件本身大小多读一个块，
    //所以只要能运行到这里，就不需要因为当前的块已满而重新realloc

    //处理当前目录
    //如果需要增加新的dabatblock：
    int new_datablock_index=-2;
    int increase = ((inode_disk_table[directory_inode_index].file_size % BLOCK_SIZE) == 0);
    if(increase){
        new_datablock_index = apply_available_bit(datablock_bitmap);
        if(new_datablock_index == -1){
            free(directory);
            return -ENOSPC;
        }
        else{
            set_bitmap(datablock_bitmap,new_datablock_index,USED);
        }
    
    }
    if(increase){
        //如果需要增加新的dabatblock：暂不处理
    }


    //修改目录的inode
    int file_size = inode_disk_table[directory_inode_index].file_size + sizeof(dentry);
    inode_disk_table[directory_inode_index].file_size = file_size;
    inode_disk_table[directory_inode_index].file_indirect_blocks_num = (file_size_to_blocks_num(file_size)>DIRECT_DATA_BLOCKS_NUM)? file_size_to_blocks_num(file_size)-DIRECT_DATA_BLOCKS_NUM :0;
    inode_disk_table[directory_inode_index].dentry_num += 1;
    inode_disk_table[directory_inode_index].blocks_number = file_size_to_blocks_num(file_size);
    inode_disk_table[directory_inode_index].last_modified_time = time(NULL);

    //添加新的目录项
    strcpy(directory->dentry_array[inode_disk_table[directory_inode_index].dentry_num-1].file_name,file_name);
    directory->dentry_array[inode_disk_table[directory_inode_index].dentry_num-1].inode = file_inode_index;

    //写入devic
    write_inode_disk_table();
    write_file(directory,directory_inode_index);
    device_flush();

    free(directory);
    return file_inode_index;

}

//返回被删除的dentry的inode_index
int remove_dentry_in_directory_by_inode(int directory_inode_index,char file_name[]){
    directory_t* directory=(directory_t*)malloc(file_size_to_blocks_num_one_more(inode_disk_table[directory_inode_index].file_size)*BLOCK_SIZE);
    if(directory == NULL){
        return -ENOMEM;
    }
    read_file(directory,directory_inode_index);

    inode_disk_t* inode = &inode_disk_table[directory_inode_index];
    //查找目录项
    int find = NOT_FIND;
    int i;
    for(i=2 ; i< inode->dentry_num ; ++i){
        if(strcmp(file_name,directory->dentry_array[i].file_name) == 0){
            find = FIND;
            break;
        }
        else
            continue;
    }

    if(!find){
        return -ENOENT;
    }


    //如果查到，把后面的挪到前面
    //directory->dentry_array[inode->dentry_num] = {.file_name ={0}, .inode=0}
    int remove_file_inode_index = directory->dentry_array[i].inode;
    memset(&directory->dentry_array[inode->dentry_num],0,sizeof(dentry));
    int location = i;
    for( ; i<inode->dentry_num ; ++i){
        directory->dentry_array[i] = directory->dentry_array[i+1];
    }
    --inode->dentry_num;
    inode->file_size -= sizeof(dentry);
    inode->last_modified_time = time(NULL);

    //回收空间交给专门的回收空间函数
    
    //写回磁盘
    write_inode_disk_table();
    write_file(directory,directory_inode_index);

    free(directory);
    return remove_file_inode_index;

}

/*
//文件的datablock_num和file_size只包括文件正文数据的datablock_num，不包括一级间接块、二级间接块
int file_increase_block_num(int file_inode_index,int new_size){
    inode_disk_t* inode = &inode_disk_table[file_inode_index];
    if(new_size <= inode->blocks_number * BLOCK_SIZE)
        return 0;
    //块数计算
    //使用的块的种类：
    //直接块：在direct_block[]里面索引的【数据块】                  //direct_datablock
    //间接数据块：通过二级间接【索引块】索引到的【数据块】          //indirect_datablock
    //一级间接索引块：indirect_block索引到的一个【索引块】          //first_indirect_block
    //二级级间接索引块：用过一级间接【索引块】索引到的一个【索引块】//second_indirect_block

    //四种块旧的数量
    //四种块新的数量
    //旧的【数据块】总数量（非索引块）
    //新的【数据块】总数量（非索引块）
    
    int new_datablock_num = ceil_division(new_size,BLOCK_SIZE);
    int old_datablock_num = inode->blocks_number;
    int add_datablock_num = new_datablock_num - old_datablock_num; 

    int old_offset_in_direct_datablock_table          = (old_datablock_num <  DIRECT_DATA_BLOCKS_NUM)?old_datablock_num : DIRECT_DATA_BLOCKS_NUM;
    int old_offset_in_first_indirect_datablock_table  = (old_datablock_num >= DIRECT_DATA_BLOCKS_NUM)? ceil_division(old_datablock_num - DIRECT_DATA_BLOCKS_NUM , BLOCK_SIZE / sizeof(int)) : 0;
    int old_offset_in_second_indirect_datablock_table = (old_datablock_num >= DIRECT_DATA_BLOCKS_NUM)?(old_datablock_num - DIRECT_DATA_BLOCKS_NUM) % (BLOCK_SIZE / sizeof(int)) : 0;

    int add_direct_datablock_num;
    int add_new_first_indirect_datablock_num;
    int add_new_second_indirect_datablock_num;

    if(old_datablock_num < DIRECT_DATA_BLOCKS_NUM && new_datablock_num <= DIRECT_DATA_BLOCKS_NUM){
        add_direct_datablock_num              = new_datablock_num - old_datablock_num;
        add_new_first_indirect_datablock_num  = 0;
        add_new_second_indirect_datablock_num = 0;
    }
    else if(old_datablock_num <= DIRECT_DATA_BLOCKS_NUM && new_datablock_num > DIRECT_DATA_BLOCKS_NUM){
        add_direct_datablock_num              = DIRECT_DATA_BLOCKS_NUM - old_datablock_num;
        add_new_first_indirect_datablock_num  = 1;
        add_new_second_indirect_datablock_num = new_datablock_num - DIRECT_DATA_BLOCKS_NUM;
    }
    else if(old_datablock_num > DIRECT_DATA_BLOCKS_NUM && new_datablock_num > DIRECT_DATA_BLOCKS_NUM){
        add_direct_datablock_num              = 0;
        add_new_first_indirect_datablock_num  = 0;
        add_new_second_indirect_datablock_num = new_datablock_num - old_datablock_num;

    }
    else
        ;




    ///////////////////////////////////////////
    //申请文件数据的数据块（包括直接数据块、间接数据块）
    //申请一级间接索引块
    //申请二级间接索引块
    ///////////////////////////////////////////

    //申请文件数据的数据块（包括直接数据块、间接数据块）
    int i;
    int new_add_datablock_index[add_datablock_num];
    for(i=0; i<add_datablock_num ; ++i){
        new_add_datablock_index[i] = apply_available_bit(datablock_bitmap);
        if(new_add_datablock_index[i] == -1){
            return -ENOSPC; // 可用空间不足
        }
    }
    //申请一级间接索引块
    int new_first_indirect_datablock_index;
    if(add_new_first_indirect_datablock_num > 0 ){
        new_first_indirect_datablock_index = apply_available_bit(datablock_bitmap);
        if(new_first_indirect_datablock_index == -1)
            return -ENOSPC;
    }
    //申请二级间接索引块
    int new_second_indirect_datablock_index[add_new_second_indirect_datablock_num];
    for(i=0 ; i<add_new_second_indirect_datablock_num ; ++i){
        new_second_indirect_datablock_index[i] = apply_available_bit(datablock_bitmap);
        if(new_second_indirect_datablock_index[i] == -1){
            return -ENOSPC;
        }
    }
    

    ///////////////////////////////////////////
    //注册数据数据块（包括直接数据块、间接数据块）
    //注册一级间接索引块
    //注册二级间接索引块
    ///////////////////////////////////////////

    //注册数据数据块（包括直接数据块、间接数据块）
    for(i=0 ; i<add_datablock_num ; ++i){
        set_bitmap(datablock_bitmap,new_add_datablock_index[i],USED);
    }
    //注册一级间接索引块
    for(i=0 ; i<add_new_first_indirect_datablock_num  ; ++i){
        set_bitmap(datablock_bitmap,new_first_indirect_datablock_index,USED);
    }
    //注册二级间接索引块
    for(i=0 ; i<add_new_second_indirect_datablock_num ; ++i){
        set_bitmap(datablock_bitmap,new_second_indirect_datablock_index[i],USED);
    }


    ///////////////////////////////////////////
    //在结构体的 直接索引块数组 里添加 直接数据块
    //在结构体的 间接索引块号   里添加 一级间接索引块
    //在   一级间接索引块 里添加 二级间接索引块
    //在   二级间接索引块 里添加 间接数据块
    ///////////////////////////////////////////

    //在结构体的 直接索引块数组 里添加 直接数据块
    int j=0;//new_add_datablock_index[j]的角标
    for(i=0 ; i<add_direct_datablock_num ; ++i,++j){
        inode->direct_block[old_offset_in_direct_datablock_table+i] = new_add_datablock_index[j];
    }

    if(new_datablock_num > DIRECT_DATA_BLOCKS_NUM){
        //在内存里申请一级间接块、二级间接块的buffer
        indirect_block_t *first_indirect_block =(indirect_block_t*)malloc(sizeof(indirect_block_t));
        if(first_indirect_block  == NULL)
            return -ENOMEM; //内存不足
        indirect_block_t *second_indirect_block=(indirect_block_t*)malloc(sizeof(indirect_block_t));
        if(second_indirect_block == NULL)
            return -ENOMEM; //内存不足

    //在结构体的 间接索引块号   里添加 一级间接索引块
        if(add_new_first_indirect_datablock_num > 0)
            //添加一级间接块
            inode->indirect_block = new_first_indirect_datablock_index;
        
        int old_up_offset_in_first_indirect_datablock_table   = (old_datablock_num >= DIRECT_DATA_BLOCKS_NUM)? ceil_division(old_datablock_num - DIRECT_DATA_BLOCKS_NUM , BLOCK_SIZE / sizeof(int)) : 0;
        int old_down_offset_in_first_indirect_datablock_table = (old_datablock_num >= DIRECT_DATA_BLOCKS_NUM)?              (old_datablock_num - DIRECT_DATA_BLOCKS_NUM)/(BLOCK_SIZE / sizeof(int)) : 0;
        
    //在   一级间接索引块 里添加 二级间接索引块
        device_read_sector(first_indirect_block,DATA_BLOCKS_LOC+inode->indirect_block);
        for(i=0; i<add_new_second_indirect_datablock_num ; ++i){
            first_indirect_block->indirect_block_table[old_up_offset_in_first_indirect_datablock_table+i] = new_second_indirect_datablock_index[i];
        }
        device_write_sector(first_indirect_block,DATA_BLOCKS_LOC+inode->indirect_block);
        
        //将上一个不满的间接块填满
        int k;
        device_read_sector(second_indirect_block , DATA_BLOCKS_LOC+first_indirect_block->indirect_block_table[old_down_offset_in_first_indirect_datablock_table]);
        for(k=old_offset_in_second_indirect_datablock_table ; k<BLOCK_SIZE/sizeof(int) && j< add_datablock_num ;++k,++j){
            second_indirect_block->indirect_block_table[k] = new_add_datablock_index[j];
        }
        device_write_sector(second_indirect_block , DATA_BLOCKS_LOC+first_indirect_block->indirect_block_table[old_down_offset_in_first_indirect_datablock_table]);
        
    //在   二级间接索引块 里添加 间接数据块
        for(i=0; i<add_new_second_indirect_datablock_num ; ++i){
            device_clear_sector(DATA_BLOCKS_LOC+first_indirect_block->indirect_block_table[old_up_offset_in_first_indirect_datablock_table + i]);
            device_read_sector(second_indirect_block , DATA_BLOCKS_LOC+first_indirect_block->indirect_block_table[old_up_offset_in_first_indirect_datablock_table + i]);
            for(k=0; k<BLOCK_SIZE/sizeof(int) && j< add_datablock_num ; ++k,++j){
                second_indirect_block->indirect_block_table[k]=new_add_datablock_index[j];
            }
            device_write_sector(second_indirect_block , DATA_BLOCKS_LOC+first_indirect_block->indirect_block_table[old_up_offset_in_first_indirect_datablock_table + i]);
        }            

        free(first_indirect_block);
        free(second_indirect_block);
    }
    //更新inode统计信息
    inode->file_size = new_size;
    inode->blocks_number = new_datablock_num;

    //写回disk：datablock_bitmap，inode，（所有间接块在前面已经写入了）
    write_inode_disk_table();
    write_bitmap(datablock_bitmap,"datablock");

    //将新添加的正文数据块清零
    int new_datablock_index_array[new_datablock_num];
    get_file_datablock_index_array(new_datablock_index_array,file_inode_index);
    int m;
    for(m=old_datablock_num ; m<new_datablock_num ; ++m){
        device_clear_sector(new_datablock_index_array[m]);
    }

    return 0;
}
*/





int file_increase_block_num(int file_inode_index,int new_size){

//read_debug_directory();
//read_debug_indirect_block();

    inode_disk_t* inode = &inode_disk_table[file_inode_index];
    if(new_size <= inode->blocks_number * BLOCK_SIZE)
        return 0;
    //块数计算
    //使用的块的种类：
    //直接块：在direct_block[]里面索引的【数据块】                  //direct_datablock
    //间接数据块：通过二级间接【索引块】索引到的【数据块】          //indirect_datablock
    //一级间接索引块：indirect_block索引到的一个【索引块】          //first_indirect_block
    //二级级间接索引块：用过一级间接【索引块】索引到的一个【索引块】//second_indirect_block

    //四种块旧的数量
    //四种块新的数量
    //旧的【数据块】总数量（非索引块）
    //新的【数据块】总数量（非索引块）

    int old_size = inode->file_size;


    int old_direct_datablock_num;
    int old_indirect_datablock_num;
    int old_first_indirect_block_num;
    int old_second_indirect_block_num;
    

    int new_direct_datablock_num;
    int new_indirect_datablock_num;
    int new_first_indirect_block_num;
    int new_second_indirect_block_num;


    int old_datablock_num = ceil_division(inode->file_size,BLOCK_SIZE);
    int new_datablock_num = ceil_division(new_size        ,BLOCK_SIZE);

    if(old_datablock_num < DIRECT_DATA_BLOCKS_NUM && new_datablock_num <= DIRECT_DATA_BLOCKS_NUM){
        old_direct_datablock_num      = old_datablock_num;
        old_indirect_datablock_num    = 0;
        old_first_indirect_block_num  = 0;
        old_second_indirect_block_num = 0;
                                                
        new_direct_datablock_num      = new_datablock_num;
        new_indirect_datablock_num    = 0;
        new_first_indirect_block_num  = 0;
        new_second_indirect_block_num = 0;
    }
    else if(old_datablock_num <= DIRECT_DATA_BLOCKS_NUM && new_datablock_num > DIRECT_DATA_BLOCKS_NUM){
        old_direct_datablock_num      = old_datablock_num;
        old_indirect_datablock_num    = 0;
        old_first_indirect_block_num  = 0;
        old_second_indirect_block_num = 0;

        new_direct_datablock_num      = DIRECT_DATA_BLOCKS_NUM;
        new_indirect_datablock_num    = new_datablock_num - DIRECT_DATA_BLOCKS_NUM;
        new_first_indirect_block_num  = 1;
        new_second_indirect_block_num = ceil_division(new_indirect_datablock_num, BLOCK_SIZE/sizeof(int));
    }
    else if(old_datablock_num > DIRECT_DATA_BLOCKS_NUM && new_datablock_num > DIRECT_DATA_BLOCKS_NUM){
        old_direct_datablock_num      = DIRECT_DATA_BLOCKS_NUM;
        old_indirect_datablock_num    = old_datablock_num - DIRECT_DATA_BLOCKS_NUM;
        old_first_indirect_block_num  = 1;
        old_second_indirect_block_num = ceil_division(old_indirect_datablock_num, BLOCK_SIZE/sizeof(int));

        new_direct_datablock_num      = DIRECT_DATA_BLOCKS_NUM;
        new_indirect_datablock_num    = new_datablock_num - DIRECT_DATA_BLOCKS_NUM;
        new_first_indirect_block_num  = 1;
        new_second_indirect_block_num = ceil_division(new_indirect_datablock_num, BLOCK_SIZE/sizeof(int));
    }
    else
        ;

    int add_datablock_num             = new_datablock_num             - old_datablock_num               ;
    int add_direct_datablock_num      = new_direct_datablock_num      - old_direct_datablock_num        ;
    int add_indirect_datablock_num    = new_indirect_datablock_num    - old_indirect_datablock_num      ;
    int add_first_indirect_block_num  = new_first_indirect_block_num  - old_first_indirect_block_num    ;
    int add_second_indirect_block_num = new_second_indirect_block_num - old_second_indirect_block_num   ;

    int add_all = add_datablock_num + add_first_indirect_block_num + add_second_indirect_block_num;
    if( count_bitmap(datablock_bitmap) + add_all > DATA_BLOCKS_NUM  )
    	return -ENOSPC;
    ///////////////////////////////////////////
    //申请,注册文件数据的数据块（包括直接数据块、间接数据块）
    //申请,注册一级间接索引块
    //申请,注册二级间接索引块
    ///////////////////////////////////////////

    int i;
    //申请文件数据的数据块（包括直接数据块、间接数据块）
    int new_datablock_index[add_datablock_num];
    for(i=0; i<add_datablock_num ; ++i){
        new_datablock_index[i] = apply_available_bit(datablock_bitmap);
        if(new_datablock_index[i] == -1){
            return -ENOSPC; // 可用空间不足
        }
        set_bitmap(datablock_bitmap, new_datablock_index[i] ,USED);
    }
    //申请一级间接索引块
    int new_first_indirect_block_index;
    for(i=0 ; i<add_first_indirect_block_num ; ++i ){
        new_first_indirect_block_index = apply_available_bit(datablock_bitmap);
        if(new_first_indirect_block_index == -1){
            return -ENOSPC;
        }
        set_bitmap(datablock_bitmap, new_first_indirect_block_index ,USED);
    }
    //申请二级间接索引块
    int new_second_indirect_block_index[add_second_indirect_block_num];
    for(i=0 ; i<add_second_indirect_block_num ; ++i){
        new_second_indirect_block_index[i] = apply_available_bit(datablock_bitmap);
        if(new_second_indirect_block_index[i] == -1){
            return -ENOSPC;
        }
        set_bitmap(datablock_bitmap, new_second_indirect_block_index[i] ,USED);
    }


    ///////////////////////////////////////////
    //在结构体的 直接索引块数组 里添加 直接数据块
    //在结构体的 间接索引块号   里添加 一级间接索引块
    //在   一级间接索引块 里添加 二级间接索引块
    //在   二级间接索引块 里添加 间接数据块
    ///////////////////////////////////////////

    //申请给间接块用的内存
    indirect_block_t *first_indirect_block =(indirect_block_t*)malloc(sizeof(indirect_block_t));
    if(first_indirect_block  == NULL)
        return -ENOMEM; //内存不足
    indirect_block_t *second_indirect_block=(indirect_block_t*)malloc(sizeof(indirect_block_t));
    if(second_indirect_block == NULL)
        return -ENOMEM; //内存不足

    int new_datablock_i=0,new_second_indirect_block_i=0; //新申请的数据块的index
    int j,k; //i:在直接块数组里的偏移，j：二级间接索引块在一级间接索引块里的偏移，k：间接数据块在二级间接索引块里的偏移
    //在结构体的 直接索引块数组 里添加 直接数据块
    for(i=old_datablock_num ; i<new_datablock_num && i<DIRECT_DATA_BLOCKS_NUM && new_datablock_i<add_datablock_num; ++i,++new_datablock_i){
        inode->direct_block[i] = new_datablock_index[new_datablock_i];
    }
    //在结构体的 间接索引块号   里添加 一级间接索引块
    if(add_first_indirect_block_num > 0)
        inode->indirect_block = new_first_indirect_block_index;
    
    //在   一级间接索引块 里添加 二级间接索引块
    if(add_first_indirect_block_num > 0){
//read_debug_directory();
//read_debug_indirect_block();
        device_clear_sector(DATA_BLOCKS_LOC + inode->indirect_block);    
//read_debug_directory();
//read_debug_indirect_block();
    }
    device_read_sector(first_indirect_block,DATA_BLOCKS_LOC + inode->indirect_block);
    for(j=old_second_indirect_block_num ; j<new_second_indirect_block_num ; ++j,++new_second_indirect_block_i){
        first_indirect_block->indirect_block_table[j] = new_second_indirect_block_index[new_second_indirect_block_i];
    }            
//read_debug_directory();
//read_debug_indirect_block();
    device_write_sector(first_indirect_block,DATA_BLOCKS_LOC + inode->indirect_block);
//read_debug_directory();
//read_debug_indirect_block();

    //在   二级间接索引块 里添加 间接数据块
    //先填充上一个没填满的二级间接索引块
    int old_down_offset_in_first_indirect_block = old_indirect_datablock_num / (BLOCK_SIZE/sizeof(int));
    int old_offset_in_second_indirect_block = old_indirect_datablock_num % (BLOCK_SIZE/sizeof(int));
    device_read_sector(second_indirect_block,DATA_BLOCKS_LOC + first_indirect_block->indirect_block_table[ old_down_offset_in_first_indirect_block ]);
    for(k=old_offset_in_second_indirect_block ; k<BLOCK_SIZE/sizeof(int) && new_datablock_i<add_datablock_num ; ++k,++new_datablock_i){
        second_indirect_block->indirect_block_table[k] = new_datablock_index[new_datablock_i];
    }
//read_debug_directory();
//read_debug_indirect_block();
    device_write_sector(second_indirect_block,DATA_BLOCKS_LOC + first_indirect_block->indirect_block_table[ old_down_offset_in_first_indirect_block ]);
//read_debug_directory();
//read_debug_indirect_block();
    //再填充新的二级间接索引块
    for(j=old_second_indirect_block_num; j<new_second_indirect_block_num && new_datablock_i<add_datablock_num;++j){
//read_debug_directory();
//read_debug_indirect_block();
        device_clear_sector(DATA_BLOCKS_LOC + first_indirect_block->indirect_block_table[j]);
//read_debug_directory();
//read_debug_indirect_block();
        device_read_sector(second_indirect_block,DATA_BLOCKS_LOC + first_indirect_block->indirect_block_table[j]);
        for(k=0; k<BLOCK_SIZE/sizeof(int) && new_datablock_i<add_datablock_num ; ++k,++new_datablock_i){
            second_indirect_block->indirect_block_table[k] = new_datablock_index[new_datablock_i];
        }
//read_debug_directory();
//read_debug_indirect_block();
        device_write_sector(second_indirect_block,DATA_BLOCKS_LOC + first_indirect_block->indirect_block_table[j]); 
//read_debug_directory();
//read_debug_indirect_block();
    }
    
    //更新inode统计信息
    inode->file_size = new_size;
    inode->blocks_number = new_datablock_num;
    inode->last_modified_time = time(NULL);

    //写回disk：datablock_bitmap，inode，（所有间接块在前面已经写入了）
//read_debug_directory();
//read_debug_indirect_block();
    write_inode_disk_table();
//read_debug_directory();
//read_debug_indirect_block();
    write_bitmap(datablock_bitmap,"datablock");
//read_debug_directory();
//read_debug_indirect_block();

    //将新添加的正文数据块清零
    int new_datablock_index_array[new_datablock_num];
    get_file_datablock_index_array(new_datablock_index_array,file_inode_index);
    int m;
    for(m=old_datablock_num ; m<new_datablock_num ; ++m){
//read_debug_directory();
//read_debug_indirect_block();
        device_clear_sector(DATA_BLOCKS_LOC + new_datablock_index_array[m]);
//read_debug_directory();
//read_debug_indirect_block();
    }

    free(first_indirect_block);
    free(second_indirect_block);
    
    return 0;
}

int file_decrease_block_num(int file_inode_index,int new_size){
    inode_disk_t * inode = &inode_disk_table[file_inode_index];
    if(new_size>= inode->file_size)
        return 0;

    //读取所有间接块号、datablock块号
    int old_second_indirect_blocks_num = count_second_indirect_blocks_num(inode->file_size);
    int old_data_blocks_num = inode->blocks_number;

    int old_second_indirect_block_index_array[old_second_indirect_blocks_num];
    int old_data_block_index_array[old_data_blocks_num];

    get_file_indirect_block_index_array(old_second_indirect_block_index_array,file_inode_index);
    get_file_datablock_index_array(old_data_block_index_array,file_inode_index);

    ///////////////////////////////////////////
    //回收一级间接块
    //回收二级间接块
    //回正文收数据块
    ///////////////////////////////////////////

    //回收一级间接块
    int old_size = inode->file_size;
    int new_data_blocks_num = ceil_division(new_size,BLOCK_SIZE);
    if(old_data_blocks_num>DIRECT_DATA_BLOCKS_NUM && new_data_blocks_num<=DIRECT_DATA_BLOCKS_NUM)
        set_bitmap(datablock_bitmap,inode->indirect_block,UNUSED);

    //回收二级间接块
    int new_second_indirect_blocks_num = count_second_indirect_blocks_num(new_size);
    int i;
    for(i=new_second_indirect_blocks_num ; i<old_second_indirect_blocks_num ; ++i){
        set_bitmap(datablock_bitmap,old_second_indirect_block_index_array[i],UNUSED);
    }

    //回正文收数据块
    for(i=new_data_blocks_num ; i<old_data_blocks_num ; ++i){
        set_bitmap(datablock_bitmap,old_data_block_index_array[i],UNUSED);
    }


    inode->file_size = new_size;
    inode->blocks_number = new_data_blocks_num;
    inode->last_modified_time = time(NULL);

    //bitmap,inode数组写回disk
    write_bitmap(datablock_bitmap,"datablock");
    write_inode_disk_table();
}

//回收inode和空间
int recycle_inode_and_space(int inode_index){

    inode_disk_t* inode = &inode_disk_table[inode_index];

    //回收空间
    int datablock_index_array[inode->blocks_number];
    get_file_datablock_index_array(datablock_index_array,inode_index);
    int i;
    //回收所有数据块
    for(i=0; i<inode->blocks_number ;++i){
        set_bitmap(datablock_bitmap,datablock_index_array[i],UNUSED);
    }
    //回收间接块
    if(inode->blocks_number > DIRECT_DATA_BLOCKS_NUM)
        set_bitmap(datablock_bitmap , inode->indirect_block , UNUSED);
    //回收inode
    set_bitmap(inode_bitmap,inode_index,UNUSED);
    inode->file_size = 0;
    inode->file_indirect_blocks_num = 0;
    inode->dentry_num = 0;
    inode->file_type = NORMAL_FILE;
    inode->blocks_number = 0;
    memset(inode->direct_block,0,DIRECT_DATA_BLOCKS_NUM*sizeof(int));
    inode->indirect_block = 0;
    inode->links = 0;

    write_bitmap(inode_bitmap,"inode");
    write_bitmap(datablock_bitmap,"datablock");
	write_inode_disk_table();
	    
    return 0;

}

int apply_available_file_info(){
    int file_info_index=-1;
    int i;
    for(i=0;i<MAX_OPEN_FILE;++i){
        if(file_info_table[i].used == UNUSED){
            file_info_index = i;
            break;
        }
        else
            continue;
    }
    return file_info_index;
}

int p6fs_mkdir(const char *path, mode_t mode)
{
     /*do path parse here
      create dentry  and update your index*/
    path_analizing_result result;
    char last_file_name[sizeof(path)];
    int second_last_inode_index,last_inode_index;
    result = path_analize(path,last_file_name,&second_last_inode_index,&last_inode_index);
    int new_file_inode_index;
    if(result == mid_not_find){
        printf("no such path\n");
        return -ENOENT;
    }
    else if(result == second_last_find___last_find){
        printf("dir has already exist\n");   
        return -EEXIST;
    }
    else{
        //处理新的dir内容
        new_file_inode_index = add_file_in_directory_by_inode(second_last_inode_index,last_file_name,DIRECTORY,0755);
        

        dentry new_dir_buf[2]={   {.file_name="..", .inode=second_last_inode_index},
                                  {.file_name="." , .inode=new_file_inode_index   }  };
        int file_size = inode_disk_table[new_file_inode_index].file_size + 2*sizeof(dentry);
        //处理新的dir inode
        inode_disk_table[new_file_inode_index].file_size = file_size;
        inode_disk_table[new_file_inode_index].file_indirect_blocks_num = (file_size_to_blocks_num(file_size)>DIRECT_DATA_BLOCKS_NUM)? file_size_to_blocks_num(file_size)-DIRECT_DATA_BLOCKS_NUM :0;
        inode_disk_table[new_file_inode_index].dentry_num += 2;
        inode_disk_table[new_file_inode_index].blocks_number = file_size_to_blocks_num(file_size);
        
        //申请新的datablock
        int new_datablock_index = apply_available_bit(datablock_bitmap);
        if(new_datablock_index == -1){
            
            return -ENOSPC;
        }
        else{
            set_bitmap(datablock_bitmap,new_datablock_index,USED);
        }
        inode_disk_table[new_file_inode_index].direct_block[0]=new_datablock_index;
        
        //写入disk
        write_bitmap(inode_bitmap,"inode");
        write_bitmap(datablock_bitmap,"datablock");
        write_inode_disk_table();
        write_file(new_dir_buf,new_file_inode_index);
        device_flush();

        return 0;
    }
    
}

int p6fs_rmdir(const char *path)
{
    int second_last_dir_inode_index,last_dir_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_dir_inode_index);

    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;

    //找到的不是个目录
    if(inode_disk_table[last_dir_inode_index].file_type != DIRECTORY)
        return -ENOTDIR;

    //找到的是根目录
    if( strcmp(path,"/")==0 )
        return -EPERM; /* Operation not permitted */

    //要删除的目录非空
    if(inode_disk_table[last_dir_inode_index].dentry_num > 2 )
        return -ENOTEMPTY;/* Directory not empty */

    //回收空间
    recycle_inode_and_space(last_dir_inode_index);
    //在父目录中取消dentry
    remove_dentry_in_directory_by_inode(second_last_dir_inode_index,file_name);

    return 0;

}

int p6fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo)
{
    //filler(buf, name, NULL, 0)
    char dir_name[MAX_FILE_NAME_LEN];
    int second_last_dir_inode_index , last_dir_inode_index;
    path_analizing_result result = path_analize(path,dir_name,&second_last_dir_inode_index,&last_dir_inode_index);
    if(result == mid_not_find || result==second_last_find___last_not_find)
        return -ENOENT;
    
    if(inode_disk_table[last_dir_inode_index].file_type != DIRECTORY)
        return -ENOTDIR;

    directory_t* directory=(directory_t*)malloc(file_size_to_blocks_num_one_more(inode_disk_table[last_dir_inode_index].file_size)*BLOCK_SIZE);
    if(directory == NULL){
        return -ENOMEM;
    }
    read_file(directory,last_dir_inode_index);

    int i;
    for(i=2; i<inode_disk_table[last_dir_inode_index].dentry_num ; ++i){
        if (filler(buf,directory->dentry_array[i].file_name,NULL,0) == 1)
            return -ENOMEM;
    }
    free(directory);
    return 0;
}

//optional
//int p6fs_opendir(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_releasedir(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo)


//file operations
int p6fs_mknod(const char *path, mode_t mode, dev_t dev)
{
 /*do path parse here
  create file*/

    //路径解析
    path_analizing_result result;
    char last_file_name[sizeof(path)];
    int second_last_inode_index,last_inode_index;
    result = path_analize(path,last_file_name,&second_last_inode_index,&last_inode_index);
    int new_file_inode_index;
    if(result == mid_not_find){
        printf("no such path\n");
        return 0;
    }
    else if(result == second_last_find___last_find){
        printf("dir has already exist\n");   
        return 0;
    }

    else{
        //处理新的dir内容
        new_file_inode_index = add_file_in_directory_by_inode(second_last_inode_index,last_file_name,NORMAL_FILE,mode);
        if(new_file_inode_index == -ENOENT)
            return -ENOENT;
        if(new_file_inode_index == -ENOSPC)
            return -ENOSPC;
        if(new_file_inode_index == -ENOMEM)
            return -ENOMEM;
        else
            return 0;
    }
    

}

int p6fs_readlink(const char *path, char *link, size_t size){
    //解析原路径
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;
    //如果link的是个目录
    //目录不允许被link，因为可能link到自己产生死循环
    if(inode_disk_table[last_file_inode_index].file_type == DIRECTORY)
        return -EISDIR;

    char symbol_link_buf[BLOCK_SIZE]={0};
    read_file(symbol_link_buf,last_file_inode_index);
    memcpy(link,symbol_link_buf,size);

    return 0;

}

int p6fs_symlink(const char *path, const char *link)
{
    //解析原路径
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;

    //解析新路径
    int new_second_last_dir_inode_index,new_last_file_inode_index;
    char new_file_name[MAX_FILE_NAME_LEN];
    path_analizing_result new_result = path_analize(link,new_file_name,&new_second_last_dir_inode_index,&new_last_file_inode_index);
    //未找到
    if(new_result == mid_not_find)
        return -ENOENT;
    //新目录中同名文件已存在
    if(new_result == second_last_find___last_find)
        return -EEXIST;


    //在当前的目录项添加文件
    int new_file_inode_index =add_file_in_directory_by_inode(new_second_last_dir_inode_index,new_file_name,SOFT_LINK,0755);
    if(new_file_inode_index == -ENOENT)
        return -ENOENT;
    if(new_file_inode_index == -ENOSPC)
        return -ENOSPC;
    if(new_file_inode_index == -ENOMEM)
        return -ENOMEM;
    
    //连接到的路径写到一个文件里
    //申请新的datablock
    int new_datablock_index = apply_available_bit(datablock_bitmap);
    if(new_datablock_index == -1){
        
        return -ENOSPC;
    }
    else{
        set_bitmap(datablock_bitmap,new_datablock_index,USED);
    }
    inode_disk_table[new_file_inode_index].direct_block[0]=new_datablock_index;

    inode_disk_table[new_file_inode_index].file_size = strlen(path);
    inode_disk_table[new_file_inode_index].blocks_number = 1;
    char symbol_link_buf[BLOCK_SIZE]={0};
    strcpy(symbol_link_buf,path);
    write_file(symbol_link_buf,new_file_inode_index);

    return 0;
}

int p6fs_link(const char *path, const char *newpath)
{

    //解析原路径
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;
    //如果link的是个目录
    //目录不允许被link，因为可能link到自己产生死循环
    if(inode_disk_table[last_file_inode_index].file_type == DIRECTORY)
        return -EISDIR;

    //解析新路径
    int new_second_last_dir_inode_index,new_last_file_inode_index;
    char new_file_name[MAX_FILE_NAME_LEN];
    path_analizing_result new_result = path_analize(newpath,new_file_name,&new_second_last_dir_inode_index,&new_last_file_inode_index);
    //未找到
    if(new_result == mid_not_find)
        return -ENOENT;
    //新目录中同名文件已存在
    if(new_result == second_last_find___last_find)
        return -EEXIST;

    //新路径目录中添加新的目录项
    add_dentry_in_directory_by_inode(new_second_last_dir_inode_index,new_file_name,last_file_inode_index);

    //++link    
    ++inode_disk_table[last_file_inode_index].links;

    //写入disk
    // write_file(new_second_last_dir_inode_index); //目录已经由add_dentry_in_directory_by_inode写入了
    write_inode_disk_table();

    return 0;
}

int p6fs_unlink(const char *path)
{
    //解析路径
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;
    //如果link的是个目录
    //目录不允许被link，因为可能link到自己产生死循环
    if(inode_disk_table[last_file_inode_index].file_type == DIRECTORY)
        return -EISDIR;

    //删除目录项
    remove_dentry_in_directory_by_inode(second_last_dir_inode_index,file_name);

    //如果对应的文件inode没有连接则回收inode和空间
    --inode_disk_table[last_file_inode_index].links;
    if( inode_disk_table[last_file_inode_index].links == 0){
        recycle_inode_and_space(last_file_inode_index);
    }

    //写入disk
    write_inode_disk_table();

    return 0;
    
}

int p6fs_open(const char *path, struct fuse_file_info *fileInfo)
{
 /*
  Implemention Example:
  S1: look up and get dentry of the path
  S2: create file handle! Do NOT lookup in read() or write() later
  */
    

    //assign and init your file handle
    struct file_info *fi;
    
    //check open flags, such as O_RDONLY
    //O_CREATE is tansformed to mknod() + open() by fuse ,so no need to create file here
    /*
     if(fileInfo->flags & O_RDONLY){
     fi->xxxx = xxxx;
     }
     */
    int last_file_inode_index,second_last_dir_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    if(result == mid_not_find || result == second_last_find___last_not_find){
        return -ENOENT;
    }
    
    int new_file_info_index = apply_available_file_info();
    if(new_file_info_index == -1)
    	return -EMFILE;
    
    fi = &file_info_table[new_file_info_index];
    fi->inode_mem = &inode_mem_table[last_file_inode_index];
    fi->used = USED;
    fi->status = fileInfo->flags;
    

/*
    if((fi->status & 3) != O_RDONLY)
        return -EACCES;
*/
    fileInfo->fh = (uint64_t)fi;

    return 0;
}

int p6fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    /* get inode from file handle and do operation*/
    //解析路径
//read_debug_directory();
//read_debug_indirect_block();
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;
    //如果是个目录
    if(inode_disk_table[last_file_inode_index].file_type == DIRECTORY)
        return -EISDIR;

    inode_disk_t* inode=&inode_disk_table[last_file_inode_index];
    if( ceil_division(size+offset,BLOCK_SIZE) > inode->blocks_number )
        return -EPERM;

    char* file_buf = (char*)malloc((inode->blocks_number+1) * BLOCK_SIZE);
    if(file_buf == NULL )
        return -ENOMEM;

    int ok_size = (size < inode->file_size - offset)?size:inode->file_size - offset;
    read_file(file_buf,last_file_inode_index);
    memcpy( buf ,file_buf+offset , ok_size);
    inode->last_access_time = time(NULL);
//read_debug_directory();
//read_debug_indirect_block();
    write_inode_disk_table();
//read_debug_directory();
//read_debug_indirect_block();

    free(file_buf);
    return ok_size;
    
}

int p6fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{

//read_debug_directory();
//read_debug_indirect_block();

    /* get inode from file handle and do operation*/
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;
    //如果是个目录
    if(inode_disk_table[last_file_inode_index].file_type == DIRECTORY)
        return -EISDIR;

    inode_disk_t* inode=&inode_disk_table[last_file_inode_index];
    if(size+offset > inode->file_size){
//read_debug_directory();
//read_debug_indirect_block();
        if( file_increase_block_num(last_file_inode_index , size+offset) == -ENOSPC )
            return -ENOSPC;
    }
//read_debug_directory();
//read_debug_indirect_block();

    char* file_buf = (char*)malloc((inode->blocks_number+1) * BLOCK_SIZE);
    if(file_buf == NULL )
        return -ENOMEM;
    memset(file_buf,0,(inode->blocks_number+1) * BLOCK_SIZE);

    int ok_size = (size < inode->file_size - offset)?size:inode->file_size - offset;
    read_file(file_buf,last_file_inode_index);
    memcpy(file_buf+offset , buf , size);
    write_file(file_buf,last_file_inode_index);
//read_debug_directory();
//read_debug_indirect_block();

    inode->last_access_time = time(NULL);
    inode->last_modified_time = time(NULL);
    inode->file_size = (size+offset > inode->file_size)?size+offset : inode->file_size;
    write_inode_disk_table();
//read_debug_directory();
//read_debug_indirect_block();

    free(file_buf);
    return size;

}

int p6fs_truncate(const char *path, off_t newSize)
{
    //解析路径
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;
    //如果truncate的是个目录
    if(inode_disk_table[last_file_inode_index].file_type == DIRECTORY)
        return -EISDIR;

    inode_disk_t* inode = &inode_disk_table[last_file_inode_index];   
    if(newSize > inode->file_size){
        if(file_increase_block_num(last_file_inode_index,newSize) == -ENOSPC)
            return -ENOSPC;
    }
    else if(newSize < inode->file_size)
        file_decrease_block_num(last_file_inode_index,newSize);

    inode->last_access_time = time(NULL);
    inode->last_modified_time = time(NULL);
    return 0;
}

//optional
//p6fs_flush(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
int p6fs_release(const char *path, struct fuse_file_info *fileInfo)
{
    /*release fd*/
    struct file_info *fi = fileInfo->fh;
    
    int last_file_inode_index,second_last_dir_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    if(result == mid_not_find || result == second_last_find___last_not_find){
        return -ENOENT;
    }

    else{
        fi->inode_mem = NULL;
        fi->used = UNUSED;
        fi->status = 0;
    }
    fileInfo->fh = NULL;

    return 0;


}

int p6fs_getattr(const char *path, struct stat *statbuf)
{
    /*stat() file or directory */
    /*stat() file or directory */
//read_debug_directory();
//read_debug_indirect_block();

    memset(statbuf, 0, sizeof(struct stat));
    
    int file_inode_index,second_last_dir_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_index,&file_inode_index);


    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;
    

    /*
    else{
        inode_disk_t inode_disk = inode_disk_table[file_inode_index];
        if(inode_disk.file_type == DIRECTORY){
            statbuf->st_mode = S_IFDIR | 755;
            statbuf->st_nlink = inode_disk.links;
        }
        else{
            statbuf->st_mode = S_IFREG | 0444;
            statbuf->st_nlink = 1;
            statbuf->st_size = inode_disk.file_size;
        }
    }*/
    inode_disk_t* inode = &inode_disk_table[file_inode_index];
    if(inode->file_type == DIRECTORY){
        statbuf->st_mode = S_IFDIR | inode->mode;
        statbuf->st_nlink = inode->links;
        statbuf->st_size = inode->file_size;
    }
    else if(inode->file_type == NORMAL_FILE){
        statbuf->st_mode = S_IFREG | inode->mode;
        statbuf->st_nlink = inode->links;
        statbuf->st_size = inode->file_size;
    }
    else{
        statbuf->st_mode = S_IFLNK | inode->mode;
        statbuf->st_nlink = inode->links;
        statbuf->st_size = inode->file_size;
    }
    statbuf->st_ctim.tv_sec = inode->create_time;
    statbuf->st_atim.tv_sec = inode->last_access_time;
    statbuf->st_mtim.tv_sec = inode->last_modified_time;

    return 0;
}

int p6fs_utime(const char *path, struct utimbuf *ubuf){
    //解析原路径
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;

    inode_disk_t* inode=&inode_disk_table[last_file_inode_index];

    ubuf->actime  = inode->last_access_time;
    ubuf->modtime = inode->last_modified_time;

    return 0;
}//optional


int p6fs_chmod(const char *path, mode_t mode){
    //解析原路径
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;

    inode_disk_t* inode=&inode_disk_table[last_file_inode_index];
    inode->mode = mode;
    inode->last_modified_time = time(NULL);

    write_inode_disk_table();
    return 0;
} //optional

/*int p6fs_chown(const char *path, uid_t uid, gid_t gid);//optional
*/

int p6fs_rename(const char *path, const char *newpath)
{
    //解析原路径
    int second_last_dir_inode_index,last_file_inode_index;
    char file_name[MAX_FILE_NAME_LEN];
    path_analizing_result result = path_analize(path,file_name,&second_last_dir_inode_index,&last_file_inode_index);
    //未找到
    if(result == mid_not_find || result == second_last_find___last_not_find)
        return -ENOENT;
    //如果要重命名的是根目录
    if( strcmp(path,"/")==0 )
        return -EPERM; /* Operation not permitted */


    //解析新路径
    int new_second_last_dir_inode_index,new_last_file_inode_index;
    char new_file_name[MAX_FILE_NAME_LEN];
    path_analizing_result new_result = path_analize(newpath,new_file_name,&new_second_last_dir_inode_index,&new_last_file_inode_index);
    //未找到
    if(new_result == mid_not_find)
        return -ENOENT;
    //新目录中同名文件已存在
    if(new_result == second_last_find___last_find)
        return -EEXIST;

    //在原路径倒数第二级目录里删除dentry
    remove_dentry_in_directory_by_inode(second_last_dir_inode_index,file_name);

    //在新路径倒数第二级目录添加新dentry
    add_dentry_in_directory_by_inode(new_second_last_dir_inode_index,new_file_name,last_file_inode_index);

    return 0;
}

int p6fs_statfs(const char *path, struct statvfs *statInfo)
{
    /*print fs status and statistics */
    int used_inode_num=count_bitmap(inode_bitmap);
    int used_datablocks_num =count_bitmap(datablock_bitmap);
    int used_space = used_datablocks_num * 4;

	statInfo->f_bavail = DATA_BLOCKS_NUM; 
    statInfo->f_bfree  = DATA_BLOCKS_NUM - used_datablocks_num ;
    statInfo->f_blocks = DATA_BLOCKS_NUM;       // total blocks
    statInfo->f_bsize  = SECTOR_SIZE;
    statInfo->f_ffree  = INODE_NUM - used_inode_num;
    statInfo->f_files  = INODE_NUM;

    return 0;
}

int my_mount(){

    //debug
    read_inode_disk_table();

    char block_buf[4*K];
    
    //read superblock
    device_read_sector(block_buf,SUPER_BLOCKS_LOC);
    memcpy(&superblock, block_buf, sizeof(superblock));
    
    //read bitmap
    read_bitmap(    inode_bitmap,"inode"    );
    read_bitmap(datablock_bitmap,"datablock");

    //debug
    read_inode_disk_table();
    
    //init inode_mem_table
    read_inode_disk_table();
    int i;
    for(i=0; i<INODE_NUM ; ++i){
        inode_mem_table[i].inode_disk = &inode_disk_table[i];
        inode_mem_table[i].indirect_block = NULL;
        pthread_mutex_init(&inode_mem_table[i].mutex,NULL);
    }

    //init file_info_table
    for (i = 0; i < MAX_OPEN_FILE; ++i) {
        file_info_table[i].used = UNUSED;
        file_info_table[i].inode_mem = NULL;
        file_info_table[i].status = 0;
    }

    //read root_directory
    //todo


    return 0;
}

int my_mkfs(){
    //superblock,backup_superblock
 
    char block_buf[BLOCK_SIZE] = {0};
    superblock_t init_superblock;

    init_superblock.magic_num                   = MAGIC_NUMBER;          
    init_superblock.total_blocks_num            = TOTAL_BLOCKS_NUM;  //how many blocks the file-system total use
    init_superblock.total_data_size             = TOTAL_DATA_SIZE;
    init_superblock.root_dir_inode              = 0;    //root directory use inode 0
    init_superblock.superblock_blocks_num       = SUPER_BLOCKS_NUM;
    init_superblock.superblock_blocks_loc       = SUPER_BLOCKS_LOC;
    init_superblock.backup_blocks_num           = BACKUP_SUPER_BLOCKS_NUM;
    init_superblock.backup_blocks_loc           = BACKUP_SUPER_BLOCKS_LOC;
    init_superblock.inode_bitmap_blocks_num     = INODE_BITMAP_BLOCKS_NUM;
    init_superblock.inode_bitmap_blocks_loc     = INODE_BITMAP_BLOCKS_LOC;
    init_superblock.datablock_bitmap_blocks_num = DATA_BITMAP_BLOCKS_NUM;
    init_superblock.datablock_bitmap_blocks_loc = DATA_BITMAP_BLOCKS_LOC;
    init_superblock.inode_blocks_num            = INODE_BLOCKS_NUM;
    init_superblock.inode_blocks_loc            = INODE_BLOCKS_LOC;
    init_superblock.inode_num                   = INODE_NUM;
    init_superblock.datablock_blocks_num        = INODE_BLOCKS_NUM;
    init_superblock.datablock_blocks_loc        = INODE_BLOCKS_LOC;

    memcpy(block_buf,&init_superblock,sizeof(superblock_t));

    device_write_sector(block_buf,SUPER_BLOCKS_LOC);
    device_write_sector(block_buf,BACKUP_SUPER_BLOCKS_LOC);
    
    //inode_table
    inode_disk_t* init_inode_disk_table;
    init_inode_disk_table = (inode_disk_t*)malloc(INODE_NUM * sizeof(inode_disk_t));
    if(init_inode_disk_table == NULL) 
        return 0;

    int i;
    for(i=0;i<INODE_NUM;++i){
        init_inode_disk_table[i].file_size                  =0;
        init_inode_disk_table[i].file_indirect_blocks_num   =0;
        init_inode_disk_table[i].dentry_num                 =0;
        init_inode_disk_table[i].file_type                  =NORMAL_FILE;
        init_inode_disk_table[i].blocks_number              =0;
        init_inode_disk_table[i].links                      =0;

        init_inode_disk_table[i].last_access_time           =time(NULL);      
        init_inode_disk_table[i].last_modified_time         =time(NULL);
        init_inode_disk_table[i].create_time                =time(NULL);
        init_inode_disk_table[i].mode                       =0;

    }

    //root directory
    init_inode_disk_table[0].file_size  += 2*sizeof(dentry);
    init_inode_disk_table[0].dentry_num += 2;
    init_inode_disk_table[0].file_type = DIRECTORY;
    init_inode_disk_table[0].blocks_number = 1;
    init_inode_disk_table[0].direct_block[0] = 0;
    init_inode_disk_table[0].links = 1;
    init_inode_disk_table[i].mode  =0755;


    memset(block_buf,0,BLOCK_SIZE);
    dentry init_root_dentrys[2] = { 
                                    {.file_name="..",.inode=0},
                                    {.file_name="." ,.inode=0}
                                  };
    memcpy(block_buf,&init_root_dentrys,sizeof(init_root_dentrys));

    device_write_multi_sector(init_inode_disk_table,INODE_BLOCKS_LOC,INODE_BLOCKS_NUM);
    device_write_sector(block_buf,DATA_BLOCKS_LOC);

    device_flush();
    read_inode_disk_table();

    //bitmap
    char init_inode_bitmap    [INODE_BITMAP_BLOCKS_NUM * BLOCK_SIZE] = {0};
    char init_datablock_bitmap[DATA_BITMAP_BLOCKS_NUM  * BLOCK_SIZE] = {0};
    set_bitmap(init_inode_bitmap,0,USED);
    set_bitmap(init_datablock_bitmap,0,USED);
    write_bitmap(init_inode_bitmap    ,"inode"    );
    write_bitmap(init_datablock_bitmap,"datablock");
    device_flush();
    free(init_inode_disk_table);
    printf("****  mkfs success  ****\n");

    return 0;

}

void* p6fs_init(struct fuse_conn_info *conn)
{
    /*init fs
     think what mkfs() and mount() should do.
     create or rebuild memory structures.
     
     e.g
     S1 Read the magic number from disk
     S2 Compare with YOUR Magic
     S3 if(exist)
        then
            mount();
        else
            mkfs();
     */
    
    
    /*HOWTO use @return
     struct fuse_context *fuse_con = fuse_get_context();
     fuse_con->private_data = (void *)xxx;
     return fuse_con->private_data;
     
     the fuse_context is a global variable, you can use it in
     all file operation, and you could also get uid,gid and pid
     from it.
     
     */

    device_open(DISK_PATH);
/*
    inode_bitmap     = (char*)malloc(INODE_BITMAP_BLOCKS_NUM * BLOCK_SIZE);
    datablock_bitmap = (char*)malloc(DATA_BITMAP_BLOCKS_NUM * BLOCK_SIZE);
    inode_disk_table = (inode_disk_t*)malloc(INODE_NUM*sizeof(inode_disk_t));
    inode_mem_table  = (inode_mem_t *)malloc(INODE_NUM*sizeof(inode_mem_t ));
    file_info_table  = (file_info*)malloc(MAX_OPEN_FILE*sizeof(file_info));

    if( inode_bitmap     == NULL) 
        return 0;
    if( datablock_bitmap == NULL) 
        return 0;
    if( inode_disk_table == NULL) 
        return 0;
    if( inode_mem_table  == NULL) 
        return 0;
    if( file_info_table  == NULL) 
        return 0;
*/
    //read superblock
    char block_buf[4*K];
    device_read_sector(block_buf,SUPER_BLOCKS_LOC);
    memcpy(&superblock, block_buf, sizeof(superblock_t));
    device_read_sector(block_buf,BACKUP_SUPER_BLOCKS_LOC);
    memcpy(&backup_superblock, block_buf, sizeof(superblock_t));

    if(superblock.magic_num != MAGIC_NUMBER){
        if(backup_superblock.magic_num == MAGIC_NUMBER){
            device_write_sector(block_buf,SUPER_BLOCKS_LOC);
        }
        else
            my_mkfs();
    }

    my_mount();

init_debug_directory();
//read_debug_directory();
init_debug_indirect_block();
//read_debug_indirect_block();

    return NULL;
}

void p6fs_destroy(void* private_data)
{
    /*
     flush data to disk
     free memory
     */

    device_flush();
    device_close();
    logging_close();
}


