#ifndef INCLUDE_FS_H_
#define INCLUDE_FS_H_

#include "type.h"

#define PAGE_SIZE 4096

#define PARTITION_TYPE 0x1A

#define MBR_BOOT_SIG (0xAA55)
#define KFS_MAGIC (0x66666666)
#define OFFSET_256M (0X10000000)
#define OFFSET_512M (0x20000000)
#define OFFSET_1G (0x40000000)
#define OFFSET_2G (0x80000000)
#define OFFSET_3G (0xC0000000)

//#define FS_SIZE (0x20000000)
#define FS_SIZE (0x10000000)
#define OFFSET_FS OFFSET_256M
#define NUM_IMAP_SECTOR 1
#define SECTOR_SIZE 512
#define BLOCK_SIZE  4096
#define NUM_FD 15

#define ENTRY_SECTOR 8

#define TYPE_FILE 1
#define TYPE_DIR 2

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR 3


// ---------------
// | boot block  |  1
// ---------------
// | super block |  1
// ---------------
// | inode map   |  1
// ---------------
// | block map   |  size / BITS_PER_BLOCK
// ---------------
// | inode array |  BITS_PER_BLOCK / INODES_PER_BLOCK
// ---------------
// | data blocks |  size - all_above
// ---------------

typedef struct DPT_entry {
    uint32_t status     :8;
    uint32_t start_chs  :24;
    uint32_t type       :8;
    uint32_t end_chs    :24;
    uint32_t start_lba;
    uint32_t sector_num;
} DPT_entry_t;

#pragma pack(2)
typedef struct MBR {
    uint8_t bootstrap_code[446];
    DPT_entry_t partition_table[4];
    uint16_t boot_signature;
} MBR_t;
#pragma pack()

typedef struct super_block {
    uint32_t magic_num;             // magic_num KFS_MAGIC
    uint32_t size;                  // Size of file system (blocks)
    uint32_t n_inodes;              // Number of inodes.
    uint32_t inode_bitmap_start;    // Block number of first bitmap of inode
    uint32_t size_inode_bitmap;
    uint32_t block_bitmap_start;    // Block number of first bitmap of all block
    uint32_t size_block_bitmap;
    uint32_t inode_start;           // Block number of first inode block
    uint32_t size_inode_array;
    uint32_t data_start;            // Block number of first data block
    uint32_t n_data_blocks;         // Number of data blocks
} super_block_t;

typedef struct inode {
    uint32_t type;                  // type of file: file, folder 
    uint32_t n_links;               // number of links;
    uint32_t size;                  // size (blocks)
    uint32_t addrs[13];             // 10 direct + 1 indirect + 1 double-indirect + 1 triple-indirect
} inode_t;

typedef struct dir_entry  
{
    char name[60];
    uint32_t inode_id;
} dir_entry_t;

// inodes per block 
#define INODES_PER_BLOCK    (BLOCK_SIZE / sizeof(inode_t))
#define SECTOR_PER_BLOCK    (BLOCK_SIZE / SECTOR_SIZE)
#define BITS_PER_BLOCK      (BLOCK_SIZE * 8)
#define DENTRY_PER_BLOCK    (BLOCK_SIZE / sizeof(dir_entry_t))

#define INODE_IN_BLOCK(i, sb)   ((i) / INODES_PER_BLOCK + sb->inode_start)

extern char cur_dir_name[64];
extern int cur_dir_inode;

extern MBR_t disk_mbr;
extern DPT_entry_t disk_partition_table[4];
extern int mbr_modified;

extern int current_partition;
extern int fs_mounted;
extern super_block_t * fs_super_block;
extern void * fs_inode_bitmap;

extern int fs_super_block_modified;
extern int fs_inode_bitmap_modified;

void do_rmdir(uint32_t inode_id);
uint32_t do_find(char * test_file_name);
uint32_t do_find_dir(char * test_file_name);
void do_cd(char * test_file_name);
void do_mkdir(uint32_t fa_inode_id, char * name);
void do_touch(uint32_t fa_inode_id, char * name);
void do_ls(uint32_t inode_id);

void print_partition_info();
void clear_partition_table(int id);
void set_partition_table(int id, uint32_t start_sector, uint32_t size);
void read_mbr();
void write_mbr();

int init_file_system(int part_id);
int mount_file_system(int part_id);
int unmount_file_system();
void print_file_system_info();

void block_read(void * dest, int part_id, int block_id);
void block_write(void * src, int part_id, int block_id);
void block_clear(int part_id, int block_id);

void init_fs();

int open(char *name, uint32_t access);
int write(uint32_t fd, char *buff, uint32_t size);
int read(uint32_t fd, char *buff, uint32_t size);
int close(uint32_t fd);
int cat(char *name);

#endif