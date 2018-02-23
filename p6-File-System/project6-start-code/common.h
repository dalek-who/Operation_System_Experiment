#ifndef P6_COMMON
#define P6_COMMON

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>

// Maybe you need the pthread locks or speedup by multi-threads or background GC in task2
// check if your $(CC) add -lfuse -pthread to $(CFLAGS) automatically, if not add them manually.
#include <pthread.h>

#include "disk.h"
#include "logging.h"



/*   on-disk  data structure   */

//bytes:
#define K ( (long long int)1024             )
#define M ( (long long int)1024*1024        )
#define G ( (long long int)1024*1024*1024   )


#define BLOCK_SIZE      ( (long long int)4*K )
#define TOTAL_DATA_SIZE ( (long long int)4*G )

//file:
#define DIRECT_DATA_BLOCKS_NUM (2)
#define MAX_FILE_NAME_LEN      (64 - 4)
#define MAX_OPEN_FILE          (32)
#define MAX_FILE_BLOCKS_NUM ( DIRECT_DATA_BLOCKS_NUM + 1*K ) //3 direct_block, one page 4K/sizeof(int)==1K indirect_block, -1 is one int indirect_block_num
#define MAX_FILE_SIZE   ( (long long int) MAX_FILE_BLOCKS_NUM * 4*K       )
#define MAX_DENTRY_NUM  ( (long long int) MAX_FILE_SIZE / sizeof(dentry)  )
#define MAX_DEEPTH (20)

#define DISK_PATH    "/home/wang/Documents/OS-exp/project6-start-code-wang/vdisk"
#define MAGIC_NUMBER 0xbadb1eed



//blocks number
#define DATA_BLOCKS_NUM         ( TOTAL_DATA_SIZE / (4*K) )  //1M blocks
#define INODE_NUM               ( DATA_BLOCKS_NUM )     //1M blocks
#define INODE_BLOCKS_NUM        ( INODE_NUM*sizeof(inode_disk_t) / (4*K) ) //1K*sizeof(inode_disk_t)
#define INODE_BITMAP_BLOCKS_NUM ( INODE_NUM / (32*K) )  //32 blocks
#define DATA_BITMAP_BLOCKS_NUM  ( DATA_BLOCKS_NUM / (32*K) ) //32 blocks
#define SUPER_BLOCKS_NUM        ( 1 )
#define BACKUP_SUPER_BLOCKS_NUM ( 1 )
#define TOTAL_BLOCKS_NUM        ( SUPER_BLOCKS_NUM + BACKUP_SUPER_BLOCKS_NUM + INODE_BITMAP_BLOCKS_NUM + DATA_BITMAP_BLOCKS_NUM + INODE_BLOCKS_NUM  + DATA_BLOCKS_NUM  )

//blocks locate
//my layout:
//SUPER_BLOCKS_NUM , BACKUP_SUPER_BLOCKS_NUM , INODE_BITMAP_BLOCKS_NUM , DATA_BITMAP_BLOCKS_NUM , INODE_BLOCKS_NUM  , DATA_BLOCKS_NUM 

#define SUPER_BLOCKS_LOC        ( 0 )       
#define BACKUP_SUPER_BLOCKS_LOC ( SUPER_BLOCKS_LOC        + SUPER_BLOCKS_NUM )
#define INODE_BITMAP_BLOCKS_LOC ( BACKUP_SUPER_BLOCKS_LOC + BACKUP_SUPER_BLOCKS_NUM )
#define DATA_BITMAP_BLOCKS_LOC  ( INODE_BITMAP_BLOCKS_LOC + INODE_BITMAP_BLOCKS_NUM )
#define INODE_BLOCKS_LOC        ( DATA_BITMAP_BLOCKS_LOC  + DATA_BITMAP_BLOCKS_NUM )
#define DATA_BLOCKS_LOC         ( INODE_BLOCKS_LOC        + INODE_BLOCKS_NUM )


/*
#define INODE_BITMAP_BLOCKS_NUM 1
#define DATA_BITMAP_BLOCKS_NUM  1
#define INODE_BLOCKS_NUM       32*sizeof(inode_disk_t)/4 //32K*sizeof(inode_disk_t)/4KB blocks, sizeof(inode_disk_t) is B size
#define DATA_BLOCKS_NUM        32*1024  //32K
#define TOTAL_BLOCKS_NUM       2 + INODE_BITMAP_BLOCKS_NUM + DATA_BLOCKS_NUM + INODE_BLOCKS_NUM + DATA_BLOCKS_NUM

#define SUPER_BLOCK            0
#define INODE_BITMAP_BLOCK     1        //one block is 4KB=32K bit,support 32K datablocks
#define DATABLOCK_BITMAP_BLOCK 2
#define INODE_BLOCK            3
#define DATA_BLOCK             3+INODE_BLOCKS_NUM
#define BACKUP_BLOCK           TOTAL_BLOCKS_NUM - 1
*/


typedef struct superblock_t{
    // complete it
    int magic_num;          
    int total_blocks_num;  //how many blocks the file-system total use
    int total_data_size ;

    int root_dir_inode;    //root directory use inode 0

    int superblock_blocks_num;
    int superblock_blocks_loc;

    int backup_blocks_num;
    int backup_blocks_loc;

    int inode_bitmap_blocks_num;
    int inode_bitmap_blocks_loc;

    int datablock_bitmap_blocks_num;
    int datablock_bitmap_blocks_loc;

    int inode_blocks_num;
    int inode_blocks_loc;
    int inode_num;

    int datablock_blocks_num;
    int datablock_blocks_loc;
}superblock_t;

typedef enum {
    DIRECTORY,
    SOFT_LINK,
    NORMAL_FILE    
}type;

//size:4+4+4+4+4+4*DIRECT_DATA_BLOCKS_NUM+4+4+8*3+4 32+24=56
typedef struct inode_disk_t{
    // complete it
    int  file_size;                 //文件大小。理论上最大4GB+8KB，但是实际上不会超过空间最大容量4G
    int  file_indirect_blocks_num;  //间接块的数量（这个后来没有用上，但是改动之后可能影响到空间换算，就留下了）
    int  dentry_num;                //如果是目录，则代表目录项的数量。如果是文件，这一项为0

    type file_type;                 //文件类型。支持目录文件、软连接文件、普通文件
    int  blocks_number;             //文件数据块的数量（不包括用来索引的间接块的数量）
    int  direct_block[DIRECT_DATA_BLOCKS_NUM];  //直接块数组。里面2个直接datablock索引
    int  indirect_block;            //间接块块号。用的是二级间接索引。
    int  links;                     //连接数量

    uint64_t last_access_time;      //上次到达的时间（读、修改）
    uint64_t last_modified_time;    //上次修改时间
    uint64_t create_time;           //创建时间
    uint32_t mode;                  //权限
}inode_disk_t;


typedef struct dentry{
    // complete it
    char file_name[MAX_FILE_NAME_LEN];
    int  inode;
}dentry;

/*  in-memory data structure   */

/*
typedef struct superblock{
    struct superblock_t *sb;
    // Add what you need, Like locks
}superblock;
*/


//some special files/blocks:
typedef struct indirect_block_t {
    int indirect_block_table[4*K /sizeof(int)]; //1K
}indirect_block_t;

typedef struct directory_t{
    dentry dentry_array[MAX_DENTRY_NUM];
}directory_t; 

typedef struct soft_link{
    char path[4*K];
}soft_link;


typedef struct inode_mem_t{
    struct inode_disk_t *inode_disk;
    pthread_mutex_t mutex;
    indirect_block_t* indirect_block;
    // Add what you need, Like locks
}inode_mem_t;

/*Your file handle structure, should be kept in <fuse_file_info>->fh
 (uint64_t see fuse_common.h), and <fuse_file_info> used in all file operations  */

typedef struct file_info{
    // complete it
    inode_mem_t* inode_mem;
    int      used       ;
    uint32_t status     ;
//    status   file_status; //fuse_common.h have already defined flags
}file_info;


//Interf.  See "fuse.h" <struct fuse_operations>
//You need to implement all the interfaces except optional ones

//dir operations
int p6fs_mkdir(const char *path, mode_t mode);
int p6fs_rmdir(const char *path);
int p6fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo);
int p6fs_opendir(const char *path, struct fuse_file_info *fileInfo);//optional
int p6fs_releasedir(const char *path, struct fuse_file_info *fileInfo);//optional
int p6fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo);//optional


//file operations
int p6fs_mknod(const char *path, mode_t mode, dev_t dev);
int p6fs_symlink(const char *path, const char *link);
int p6fs_link(const char *path, const char *newpath);
int p6fs_unlink(const char *path);
int p6fs_readlink(const char *path, char *link, size_t size);//optional

int p6fs_open(const char *path, struct fuse_file_info *fileInfo);
int p6fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
int p6fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
int p6fs_truncate(const char *path, off_t newSize);
int p6fs_flush(const char *path, struct fuse_file_info *fileInfo);//optional
int p6fs_fsync(const char *path, int datasync, struct fuse_file_info *fi);//optional
int p6fs_release(const char *path, struct fuse_file_info *fileInfo);


int p6fs_getattr(const char *path, struct stat *statbuf);
int p6fs_utime(const char *path, struct utimbuf *ubuf);//optional
int p6fs_chmod(const char *path, mode_t mode); //optional
int p6fs_chown(const char *path, uid_t uid, gid_t gid);//optional

int p6fs_rename(const char *path, const char *newpath);
int p6fs_statfs(const char *path, struct statvfs *statInfo);
void* p6fs_init(struct fuse_conn_info *conn);
void p6fs_destroy(void* private_data);//optional

#endif
