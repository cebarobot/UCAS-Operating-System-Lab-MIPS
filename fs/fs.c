#include "fs.h"
#include "string.h"
#include "common.h"
#include "mm.h"
#include "bitmap.h"
#include "stdio.h"
#include "sched.h"

char cur_dir_name[64] = {"/"};
int cur_dir_inode;

MBR_t disk_mbr;
DPT_entry_t disk_partition_table[4];
int mbr_modified = 0;

int current_partition;
int fs_mounted = 0;
super_block_t * fs_super_block;
int fs_super_block_modified = 0;

void read_mbr() {
    sd_card_read(&disk_mbr, 0, SECTOR_SIZE);
    mbr_modified = 0;
    // if (disk_mbr.boot_signature != MBR_BOOT_SIG) {
    //     mbr_modified = 1;
    //     disk_mbr.boot_signature = MBR_BOOT_SIG;
    // }
    memcpy((void *) disk_partition_table, (void *) disk_mbr.partition_table, 4 * sizeof(DPT_entry_t));
}

void write_mbr() {
    memcpy((void *) disk_mbr.partition_table, (void *) disk_partition_table, 4 * sizeof(DPT_entry_t));
    sd_card_write(&disk_mbr, 0, SECTOR_SIZE);
}

void set_partition_table(int id, uint32_t start_sector, uint32_t size) {
    mbr_modified = 1;
    disk_partition_table[id].status = id == 0 ? 0x80 : 0x00;
    disk_partition_table[id].start_chs = 0xFFFFFE;
    disk_partition_table[id].type = PARTITION_TYPE;
    disk_partition_table[id].end_chs = 0xFFFFFE;
    disk_partition_table[id].start_lba = start_sector;
    disk_partition_table[id].sector_num = size;
}

void clear_partition_table(int id) {
    mbr_modified = 1;
    memset(disk_partition_table + id, 0, sizeof(DPT_entry_t));
}


// disk buff
#define DISK_BUFF_SIZE 32
int id_next_new_buff = 0;
void * disk_buff[DISK_BUFF_SIZE];
int disk_buff_using[DISK_BUFF_SIZE];
int disk_buff_dirty[DISK_BUFF_SIZE];
int disk_buff_block_id[DISK_BUFF_SIZE];
int cnt_dirty_buff;

void disk_buff_init() {
    id_next_new_buff = 0;
    for (int i = 0; i < DISK_BUFF_SIZE; i++) {
        disk_buff[i] = NULL;
        disk_buff_dirty[i] = 0;
        disk_buff_block_id[i] = 0;
        disk_buff_using[i] = 0;
    }
    cnt_dirty_buff = 0;
}

void block_read(void * dest, int part_id, int block_id) {
    int offset = disk_partition_table[part_id].start_lba * SECTOR_SIZE + block_id * BLOCK_SIZE;
    sd_card_read(dest, offset, BLOCK_SIZE);
}
void block_write(void * src, int part_id, int block_id) {
    int offset = disk_partition_table[part_id].start_lba * SECTOR_SIZE + block_id * BLOCK_SIZE;
    sd_card_write(src, offset, BLOCK_SIZE);
}
void block_clear(int part_id, int block_id) {
    int offset = disk_partition_table[part_id].start_lba * SECTOR_SIZE + block_id * BLOCK_SIZE;
    sd_card_write(zero_page, offset, BLOCK_SIZE);
}


void * block_get(int block_id, int do_read) {
    // kprintf("BUFF: get block %d\n", block_id);

    for (int i = 0; i < DISK_BUFF_SIZE; i++) {
        if (disk_buff_block_id[i] == block_id && disk_buff[i]) {
            disk_buff_using[i] = 1;
            return disk_buff[i];
        }
    }

    int buff_id = id_next_new_buff;
    // kprintf("BUFF REPLACE: buff_id = %d\n", buff_id);
    while (disk_buff_using[buff_id]) {
        buff_id = (buff_id + 1) % DISK_BUFF_SIZE;
    }

    if (disk_buff[buff_id]) {
        if (disk_buff_dirty[buff_id]) {
            block_write(disk_buff[buff_id], current_partition, disk_buff_block_id[buff_id]);
            cnt_dirty_buff -= 1;
        }
        free_page(&page_ctrl_kernel, (uint64_t) disk_buff[buff_id]);
        disk_buff[buff_id] = NULL;
        disk_buff_dirty[buff_id] = 0;
        disk_buff_block_id[buff_id] = 0;
    }

    disk_buff[buff_id] = (void *) alloc_page(&page_ctrl_kernel);
    disk_buff_block_id[buff_id] = block_id;
    if (do_read) {
        block_read(disk_buff[buff_id], current_partition, block_id);
        disk_buff_dirty[buff_id] = 0;
    } else {
        disk_buff_dirty[buff_id] = 1;
        cnt_dirty_buff += 1;
    }

    id_next_new_buff = (id_next_new_buff + 1) % DISK_BUFF_SIZE;
    return disk_buff[buff_id];
}

void block_commit(void * block, int do_write) {
    int buff_id;
    for (buff_id = 0; buff_id < DISK_BUFF_SIZE; buff_id++) {
        if (disk_buff[buff_id] == block) {
            break;
        }
    }
    // kprintf("BUFF: commit block %d\n", disk_buff_block_id[buff_id]);
    if (buff_id == DISK_BUFF_SIZE) {
        kprintf("ERROR: unloaded block\n");
        return;
    }
    if (do_write) {
        disk_buff_dirty[buff_id] = 1;
    }
    disk_buff_using[buff_id] = 0;
}

void block_sink() {
    for (int i = 0; i < DISK_BUFF_SIZE; i++) {
        if (disk_buff_dirty[i]) {
            block_write(disk_buff[i], current_partition, disk_buff_block_id[i]);
            disk_buff_dirty[i] = 0;
            cnt_dirty_buff -= 1;
        }
        kprintf("#");
    }
    kprintf("\n");
}


int mount_file_system(int part_id) {
    if (fs_mounted) {
        kprintf("error: mounted\n");
        return -1;
    }
    current_partition = part_id;

    fs_super_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fs_super_block, part_id, 1);
    if (fs_super_block->magic_num != KFS_MAGIC) {
        kprintf("%08x\n", fs_super_block->magic_num);
        free_page(&page_ctrl_kernel, (uint64_t) fs_super_block);
        kprintf("error: wrong magic\n");
        return -1;
    }

    fs_mounted = 1;
    kprintf("mount done\n");
    return 0;
}

int unmount_file_system() {
    if (!fs_mounted) {
        return -1;
    }

    block_sink();

    block_write(fs_super_block, current_partition, 1);
    free_page(&page_ctrl_kernel, (uint64_t) fs_super_block);

    fs_mounted = 0;
    return 0;
}

int init_file_system(int part_id) {
    if (fs_mounted) {
        return -1;
    }

    current_partition = part_id;
    // setup super block
    kprintf("setup super block\n");
    fs_super_block = (void *) alloc_page(&page_ctrl_kernel);
    fs_super_block->magic_num = KFS_MAGIC;
    fs_super_block->size = disk_partition_table[current_partition].sector_num / SECTOR_PER_BLOCK;
    fs_super_block->n_inodes = BITS_PER_BLOCK;
    fs_super_block->inode_bitmap_start = 2;
    fs_super_block->size_inode_bitmap = 1;
    fs_super_block->block_bitmap_start = fs_super_block->inode_bitmap_start + fs_super_block->size_inode_bitmap;
    fs_super_block->size_block_bitmap = fs_super_block->size / BITS_PER_BLOCK;
    if (fs_super_block->size % BITS_PER_BLOCK) {
        fs_super_block->size_block_bitmap += 1;
    }
    fs_super_block->inode_start = fs_super_block->block_bitmap_start + fs_super_block->size_block_bitmap;
    fs_super_block->size_inode_array = BITS_PER_BLOCK / INODES_PER_BLOCK;
    fs_super_block->data_start = fs_super_block->inode_start + fs_super_block->size_inode_array;
    fs_super_block->n_data_blocks = fs_super_block->size - fs_super_block->data_start;
    // write super block
    kprintf("write super block\n");
    block_write(fs_super_block, current_partition, 1);
    fs_super_block_modified = 0;

    // setup inode map
    kprintf("setup inode map\n");
    void * inode_bitmap = block_get(fs_super_block->inode_bitmap_start, 0);

    bitmap_init_byte(inode_bitmap, BLOCK_SIZE);
    bitmap_set(inode_bitmap, 0);

    block_commit(inode_bitmap, 1);

    // setup block map
    kprintf("setup block map\n");
    void * block_bitmap = block_get(fs_super_block->block_bitmap_start, 0);
    bitmap_init_byte(block_bitmap, BLOCK_SIZE);
    for (int i = 0; i <= fs_super_block->data_start; i++) {         // all block before data block and data block
        bitmap_set(block_bitmap, i);
    }
    block_commit(block_bitmap, 1);
    
    for (int i = 1; i < fs_super_block->size_block_bitmap; i++) {
        block_bitmap = block_get(fs_super_block->block_bitmap_start + i, 0);
        bitmap_init_byte(block_bitmap, BLOCK_SIZE);
        block_commit(block_bitmap, 1);
    }

    // setup inode
    kprintf("setup inode\n");
    inode_t * inode_block = block_get(fs_super_block->inode_start, 0);
    memset(inode_block, 0, BLOCK_SIZE);
    inode_block[0].type = TYPE_DIR;
    inode_block[0].n_links = 1;
    inode_block[0].size = 1;
    inode_block[0].addrs[0] = fs_super_block->data_start;
    block_commit(inode_block, 1);

    // setup data block
    kprintf("setup data block\n");
    dir_entry_t * data_block = block_get(fs_super_block->data_start, 0);
    memset(data_block, 0, BLOCK_SIZE);
    dir_entry_t * root_dir = data_block;
    strcpy(root_dir[0].name, ".");
    root_dir[0].inode_id = 0;
    strcpy(root_dir[1].name, "..");
    root_dir[1].inode_id = 0;
    block_commit(data_block, 1);

    fs_mounted = 1;
}

void print_partition_info() {
    kprintf("[DISK] partition infomation:\n");
    kprintf(" id  ");
    kprintf(" start (sector) ");
    kprintf(" size (sector)  ");
    kprintf("\n");
    for (int i = 0; i < 4; i++) {
        kprintf(" %d   ", i);
        if (disk_partition_table[i].type != PARTITION_TYPE) {
            kprintf("  NULL");
        } else {
            kprintf(" 0x%08x     ", disk_partition_table[i].start_lba);
            kprintf(" 0x%08x     ", disk_partition_table[i].sector_num);
        }
        kprintf("\n");
    }
}

uint32_t alloc_inode() {
    void * inode_bitmap = block_get(fs_super_block->inode_bitmap_start, 1);
    int inode_id;
    for (int i = 0; i < BITS_PER_BLOCK; i++) {
        if (bitmap_check(inode_bitmap, i) == 0) {
            inode_id = i;
            break;
        }
    }
    bitmap_set(inode_bitmap, inode_id);
    block_commit(inode_bitmap, 1);
    return inode_id;
}

void free_inode(uint32_t inode_id) {
    void * inode_bitmap = block_get(fs_super_block->inode_bitmap_start, 1);
    bitmap_clear(inode_bitmap, inode_id);
    block_commit(inode_bitmap, 1);
}

uint32_t alloc_data_block() {
    int bitmap_block_id = fs_super_block->block_bitmap_start;
    void * block_bitmap = block_get(bitmap_block_id, 1);

    int data_block_id;

    for (int i = fs_super_block->data_start; i < fs_super_block->size; i++) {
        if (i / BITS_PER_BLOCK + fs_super_block->block_bitmap_start != bitmap_block_id) {
            bitmap_block_id += 1;
            block_commit(block_bitmap, 0);
            block_bitmap = block_get(bitmap_block_id, 1);
        }
        int offset = i % BITS_PER_BLOCK;
        if (bitmap_check(block_bitmap, offset) == 0) {
            bitmap_set(block_bitmap, offset);
            data_block_id = i;
            break;
        }
    }

    block_commit(block_bitmap, 1);
    return data_block_id;
}

void free_data_block(uint32_t data_block_id) {
    int bitmap_block_id = data_block_id / BITS_PER_BLOCK + fs_super_block->block_bitmap_start;
    int offset = data_block_id % BITS_PER_BLOCK;

    void * block_bitmap = block_get(bitmap_block_id, 1);
    bitmap_clear(block_bitmap, offset);
    block_commit(block_bitmap, 1);
}

void do_ls(uint32_t inode_id) {
    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;
    
    inode_t * inode_block = block_get(inode_block_id, 1);

    inode_t * this_inode = inode_block + offset;

    dir_entry_t * dir_block = block_get(this_inode->addrs[0], 1);

    for (int i = 0; i < DENTRY_PER_BLOCK; i++) {
        if (dir_block[i].name[0] == 0) {
            break;
        }
        kprintf("%s\n", dir_block[i].name);
    }

    block_commit(inode_block, 0);
    block_commit(dir_block, 0);
}

void do_mkdir(uint32_t fa_inode_id, char * name) {
    uint32_t fa_inode_block_id = INODE_IN_BLOCK(fa_inode_id, fs_super_block);
    uint32_t fa_offset = fa_inode_id % INODES_PER_BLOCK;

    inode_t * fa_inode_block = block_get(fa_inode_block_id, 1);

    inode_t * fa_inode = fa_inode_block + fa_offset;

    dir_entry_t * fa_dir_block = block_get(fa_inode->addrs[0], 1);

    int dir_i;
    for (dir_i = 0; dir_i < DENTRY_PER_BLOCK; dir_i++) {
        if (strcmp(fa_dir_block[dir_i].name, name) == 0) {
            return;
        }
        if (fa_dir_block[dir_i].name[0] == 0) {
            break;
        }
    }

    uint32_t inode_id = alloc_inode();

    strcpy(fa_dir_block[dir_i].name, name);
    fa_dir_block[dir_i].inode_id = inode_id;
    block_commit(fa_dir_block, 1);

    block_commit(fa_inode_block, 0);
    
    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;
    
    inode_t * inode_block = block_get(inode_block_id, 1);

    inode_t * this_inode = inode_block + offset;
    
    this_inode->type = TYPE_DIR;
    this_inode->n_links = 1;
    this_inode->size = 1;
    this_inode->addrs[0] = alloc_data_block();

    block_commit(inode_block, 1);

    dir_entry_t * dir_block = block_get(this_inode->addrs[0], 0);
    memset(dir_block, 0, BLOCK_SIZE);

    strcpy(dir_block[0].name, ".");
    dir_block[0].inode_id = inode_id;
    strcpy(dir_block[1].name, "..");
    dir_block[1].inode_id = fa_inode_id;
    
    block_commit(dir_block, 1);
}

uint32_t do_find(char * test_file_name) {
    char file_name[100];
    strcpy(file_name, test_file_name);

    int error = 0;

    int inode_id;

    char * ppp = file_name;
    if (ppp[0] == '/') {
        inode_id = 0;
        ppp ++;
    } else {
        inode_id = cur_dir_inode;
    }

    inode_t * inode_block;
    dir_entry_t * dir_block;

    uint32_t inode_block_id;
    uint32_t offset;
    inode_t * this_inode;

    int is_end = 0;
    while (*ppp) {
        char *qqq;
        for (qqq = ppp; *qqq && *qqq != '/'; qqq++);

        if (*qqq == 0) {
            is_end = 1;
        }

        *qqq = 0;

        inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
        offset = inode_id % INODES_PER_BLOCK;
        
        inode_block = block_get(inode_block_id, 1);

        this_inode = inode_block + offset;

        if (this_inode->type != TYPE_DIR) {
            kprintf("ERROR: %s is not dir\n", ppp);
            error = 1;
            break;
        }

        dir_block = block_get(this_inode->addrs[0], 1);

        for (int i = 0; i < DENTRY_PER_BLOCK; i++) {
            if (dir_block[i].name[0] == 0) {
                kprintf("ERROR: can't found %s\n", ppp);
                error = 1;
                break;
            }
            if (strcmp(dir_block[i].name, ppp) == 0) {
                inode_id = dir_block[i].inode_id;
                break;
            }
        }

        if (error || is_end) {
            break;
        }

        ppp = qqq + 1;

        block_commit(inode_block, 0);
        block_commit(dir_block, 0);
    }

    if (error == 0) {
        return inode_id;
    }
    return -1;
}


uint32_t do_find_dir(char * test_file_name) {
    char file_name[100];
    strcpy(file_name, test_file_name);

    int error = 0;

    int inode_id;

    char * ppp = file_name;
    if (ppp[0] == '/') {
        inode_id = 0;
        ppp ++;
    } else {
        inode_id = cur_dir_inode;
    }

    inode_t * inode_block;
    dir_entry_t * dir_block;

    uint32_t inode_block_id;
    uint32_t offset;
    inode_t * this_inode;

    int is_end = 0;
    while (*ppp) {
        char *qqq;
        for (qqq = ppp; *qqq && *qqq != '/'; qqq++);

        if (*qqq == 0) {
            is_end = 1;
        }

        *qqq = 0;

        inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
        offset = inode_id % INODES_PER_BLOCK;
        
        inode_block = block_get(inode_block_id, 1);

        this_inode = inode_block + offset;

        if (this_inode->type != TYPE_DIR) {
            kprintf("ERROR: %s is not dir\n", ppp);
            error = 1;
            break;
        }

        dir_block = block_get(this_inode->addrs[0], 1);

        for (int i = 0; i < DENTRY_PER_BLOCK; i++) {
            if (dir_block[i].name[0] == 0) {
                kprintf("ERROR: can't found %s\n", ppp);
                error = 1;
                break;
            }
            if (strcmp(dir_block[i].name, ppp) == 0) {
                inode_id = dir_block[i].inode_id;
                break;
            }
        }

        if (error || is_end) {
            break;
        }

        ppp = qqq + 1;
        block_commit(inode_block, 0);
        block_commit(dir_block, 0);
    }

    inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    offset = inode_id % INODES_PER_BLOCK;
    
    inode_block = block_get(inode_block_id, 1);

    this_inode = inode_block + offset;

    if (this_inode->type != TYPE_DIR) {
        kprintf("ERROR: %s is not dir\n", ppp);
        error = 1;
    }

    block_commit(inode_block, 0);

    if (error == 0) {
        return inode_id;
    }
    return -1;
}


void do_cd(char * test_file_name) {
    char file_name[100];
    strcpy(file_name, test_file_name);

    int error = 0;

    int inode_id;
    char dir_name[64];

    char * ppp = file_name;
    if (ppp[0] == '/') {
        inode_id = 0;
        ppp ++;
    } else {
        inode_id = cur_dir_inode;
    }

    inode_t * inode_block;
    dir_entry_t * dir_block;

    uint32_t inode_block_id;
    uint32_t offset;
    inode_t * this_inode;

    int is_end = 0;
    while (*ppp) {
        char *qqq;
        for (qqq = ppp; *qqq && *qqq != '/'; qqq++);

        if (*qqq == 0) {
            is_end = 1;
        }

        *qqq = 0;

        inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
        offset = inode_id % INODES_PER_BLOCK;
        
        inode_block = block_get(inode_block_id, 1);

        this_inode = inode_block + offset;

        if (this_inode->type != TYPE_DIR) {
            kprintf("ERROR: %s is not dir\n", ppp);
            error = 1;
            break;
        }

        dir_block = block_get(this_inode->addrs[0], 1);

        for (int i = 0; i < DENTRY_PER_BLOCK; i++) {
            if (dir_block[i].name[0] == 0) {
                kprintf("ERROR: can't found %s\n", ppp);
                error = 1;
                break;
            }
            if (strcmp(dir_block[i].name, ppp) == 0) {
                inode_id = dir_block[i].inode_id;
                strcpy(dir_name, dir_block[i].name);
                break;
            }
        }

        if (error || is_end) {
            break;
        }

        ppp = qqq + 1;
        block_commit(inode_block, 0);
        block_commit(dir_block, 0);
    }

    inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    offset = inode_id % INODES_PER_BLOCK;
    
    inode_block = block_get(inode_block_id, 1);

    this_inode = inode_block + offset;

    if (this_inode->type != TYPE_DIR) {
        kprintf("ERROR: %s is not dir\n", ppp);
        error = 1;
    }

    if (error == 0) {
        if (inode_id) {
            strcpy(cur_dir_name, dir_name);
        } else {
            strcpy(cur_dir_name, "/");
        }
        cur_dir_inode = inode_id;
    }
    block_commit(inode_block, 0);
}


void do_rmdir(uint32_t inode_id) {
    inode_t * inode_block;
    dir_entry_t * dir_block;

    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;
    
    inode_block = block_get(inode_block_id, 1);

    inode_t * this_inode = inode_block + offset;

    dir_block = block_get(this_inode->addrs[0], 1);

    uint32_t fa_inode_id = dir_block[1].inode_id;
    
    uint32_t fa_inode_block_id = INODE_IN_BLOCK(fa_inode_id, fs_super_block);
    uint32_t fa_offset = fa_inode_id % INODES_PER_BLOCK;

    inode_t * fa_inode_block = block_get(fa_inode_block_id, 1);

    inode_t * fa_inode = fa_inode_block + fa_offset;

    dir_entry_t * fa_dir_block = block_get(fa_inode->addrs[0], 1);

    int dir_id;
    int last_dir_id;

    for (int i = 0; i < DENTRY_PER_BLOCK; i++) {
        if (fa_dir_block[i].name[0] == 0) {
            last_dir_id = i - 1;
            break;
        }
        if (fa_dir_block[i].inode_id == inode_id) {
            dir_id = i;
        }
    }

    if (dir_id == last_dir_id) {
        memset(fa_dir_block + dir_id, 0, sizeof(dir_entry_t));
    } else {
        memcpy((void *) fa_dir_block + dir_id, (void *) fa_dir_block + last_dir_id, sizeof(dir_entry_t));
        memset(fa_dir_block + last_dir_id, 0, sizeof(dir_entry_t));
    }

    block_commit(inode_block, 0);
    block_commit(dir_block, 0);
    block_commit(fa_inode_block, 0);
    block_commit(fa_dir_block, 1);
    
    free_inode(inode_id);
    free_data_block(inode_block->addrs[0]);
}

void do_touch(uint32_t fa_inode_id, char * name) {
    uint32_t fa_inode_block_id = INODE_IN_BLOCK(fa_inode_id, fs_super_block);
    uint32_t fa_offset = fa_inode_id % INODES_PER_BLOCK;

    inode_t * fa_inode_block = block_get(fa_inode_block_id, 1);

    inode_t * fa_inode = fa_inode_block + fa_offset;

    dir_entry_t * fa_dir_block = block_get(fa_inode->addrs[0], 1);

    int dir_i;
    for (dir_i = 0; dir_i < DENTRY_PER_BLOCK; dir_i++) {
        if (strcmp(fa_dir_block[dir_i].name, name) == 0) {
            return;
        }
        if (fa_dir_block[dir_i].name[0] == 0) {
            break;
        }
    }

    uint32_t inode_id = alloc_inode();

    strcpy(fa_dir_block[dir_i].name, name);
    fa_dir_block[dir_i].inode_id = inode_id;
    block_commit(fa_dir_block, 1);
    
    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;
    
    inode_t * inode_block = block_get(inode_block_id, 1);

    inode_t * this_inode = inode_block + offset;
    
    this_inode->type = TYPE_FILE;
    this_inode->n_links = 1;
    this_inode->size = 0;

    block_commit(inode_block, 1);
    block_commit(fa_inode_block, 0);
}

void print_file_system_info() {
    if (!fs_mounted) {
        kprintf("[FS] file system has not mounted\n");
        return;
    }
    kprintf("[FS] partition %d super block infomation:\n", current_partition);
    kprintf("     magic_num:     0x%08x\n", fs_super_block->magic_num);
    kprintf("     size:          0x%08x (%d) blocks\n", fs_super_block->size, fs_super_block->size);
    kprintf("     n_inodes:      0x%08x (%d)\n", fs_super_block->n_inodes, fs_super_block->n_inodes);
    kprintf("     name         start      size\n");
    kprintf("     inode map    0x%08x 0x%08x\n", fs_super_block->inode_bitmap_start, fs_super_block->size_inode_bitmap);
    kprintf("     block map    0x%08x 0x%08x\n", fs_super_block->block_bitmap_start, fs_super_block->size_block_bitmap);
    kprintf("     inode array  0x%08x 0x%08x\n", fs_super_block->inode_start, fs_super_block->size_inode_array);
    kprintf("     data block   0x%08x 0x%08x\n", fs_super_block->data_start, fs_super_block->n_data_blocks);
    kprintf("     inode_entry_size:      %d bytes\n", (uint32_t) sizeof(inode_t));
}

file_t * files;

void files_init() {
    files = (void *) alloc_page(&page_ctrl_kernel);
    memset(files, 0, PAGE_SIZE);
}

int open(char *name, uint32_t access) {
    int file_id;
    for (file_id = 0; file_id < GLOBAL_MAX_FILES; file_id++) {
        if (files[file_id].using == 0) {
            break;
        }
    }
    if (file_id == GLOBAL_MAX_FILES) {
        kprintf("ERROR: cannot open more files\n");
        return -1;
    }

    file_t * this_file = &files[file_id];
    
    int fd;
    for (fd = 0; fd < MAX_FILES; fd ++) {
        if (current_running->files[fd] == 0) {
            break;
        }
    }
    if (fd == MAX_FILES) {
        kprintf("ERROR: cannot open more files\n");
        return -1;
    }

    int inode_id = do_find(name);
    if (inode_id < 0) {
        kprintf("ERROR: something wrong when opening the file\n");
        return -1;
    }

    this_file->using = 1;
    this_file->inode_id = inode_id;
    this_file->access = access;
    this_file->offset = 0;

    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;

    inode_t * inode_block = block_get(inode_block_id, 1);
    inode_t * this_inode = inode_block + offset;

    memcpy((void*) &this_file->inode, (void*) this_inode, sizeof(inode_t));

    block_commit(inode_block, 0);

    current_running->files[fd] = this_file;

    return fd;
}

static uint32_t find_data_block(file_t * file, int offset) {
    int i_block = offset / BLOCK_SIZE;
    file->inode.size = MAX(file->inode.size, i_block + 1);

    int data_block_id = -1;

    if (i_block < 10) {
        if (file->inode.addrs[i_block] == 0) {
            file->inode.addrs[i_block] = alloc_data_block();
        }
        data_block_id = file->inode.addrs[i_block];

    } else if (i_block < 10 + ADDR_PER_BLOCK) {
        int i2_block = i_block - 10;
        uint32_t * indirect_addrs;
        int indirect_addrs_dirty = 0;

        if (file->inode.addrs[10]) {
            indirect_addrs = block_get(file->inode.addrs[10], 1);
        } else {
            file->inode.addrs[10] = alloc_data_block();
            indirect_addrs = block_get(file->inode.addrs[10], 0);
            memset(indirect_addrs, 0, sizeof(BLOCK_SIZE));
        }

        if (indirect_addrs[i2_block]) {
            data_block_id = indirect_addrs[i2_block];
        } else {
            data_block_id = alloc_data_block();
            indirect_addrs[i2_block] = data_block_id;
            indirect_addrs_dirty = 1;
        }

        block_commit(indirect_addrs, indirect_addrs_dirty);

    } else if (i_block < 10 + ADDR_PER_BLOCK + ADDR_PER_BLOCK * ADDR_PER_BLOCK) {
        int i3_block = (i_block - 10 - ADDR_PER_BLOCK) / ADDR_PER_BLOCK;
        int i4_block = (i_block - 10 - ADDR_PER_BLOCK) % ADDR_PER_BLOCK;
        uint32_t * indirect_addrs;
        uint32_t * double_addrs;
        int indirect_addrs_dirty = 0;
        int double_addrs_dirty = 0;

        if (file->inode.addrs[11]) {
            indirect_addrs = block_get(file->inode.addrs[11], 1);
        } else {
            file->inode.addrs[11] = alloc_data_block();
            
            indirect_addrs = block_get(file->inode.addrs[11], 0);
            memset(indirect_addrs, 0, sizeof(BLOCK_SIZE));
        }

        if (indirect_addrs[i3_block]) {
            double_addrs = block_get(indirect_addrs[i3_block], 1);
        } else {
            indirect_addrs[i3_block] = alloc_data_block();
            indirect_addrs_dirty = 1;

            double_addrs = block_get(indirect_addrs[i3_block], 0);
            memset(double_addrs, 0, sizeof(BLOCK_SIZE));
        }

        if (double_addrs[i4_block]) {
            data_block_id = double_addrs[i4_block];
        } else {
            data_block_id = alloc_data_block();
            double_addrs[i4_block] = data_block_id;
            double_addrs_dirty = 1;
        }

        block_commit(indirect_addrs, indirect_addrs_dirty);
        block_commit(double_addrs, double_addrs_dirty);
    }
    return data_block_id;
}

int write(uint32_t fd, char *buff, uint32_t size) {
    file_t * this_file = current_running->files[fd];

    int buff_off = 0;
    int file_off = this_file->offset;
    
    while (buff_off < size) {

        int file_end = (file_off / BLOCK_SIZE + 1) * BLOCK_SIZE;
        if (file_end > this_file->offset + size) {
            file_end = this_file->offset + size;
        }

        int copy_size = file_end - file_off;

        int data_block_id = find_data_block(this_file, file_off);
        void * data_block = block_get(data_block_id, copy_size != BLOCK_SIZE);
        // kprintf("--------------------------cpy file_off %d buff_off %d\n", (file_off) % BLOCK_SIZE, buff_off);
        memcpy(data_block + (file_off) % BLOCK_SIZE, buff + buff_off, copy_size);
        block_commit(data_block, 1);

        buff_off += copy_size;
        file_off = file_end;
    }
    this_file->offset = file_off;
    // kprintf("================%d\n", this_file->offset);
}

int read(uint32_t fd, char *buff, uint32_t size) {
    file_t * this_file = current_running->files[fd];

    int buff_off = 0;
    int file_off = this_file->offset;
    
    while (buff_off < size) {
        // kprintf("~~~~~~~~~~~~~~~~~~~~~~~~~~~cpy file_off %d buff_off %d\n", file_off, buff_off);

        int file_end = (file_off / BLOCK_SIZE + 1) * BLOCK_SIZE;
        if (file_end > this_file->offset + size) {
            file_end = this_file->offset + size;
        }

        int copy_size = file_end - file_off;

        int data_block_id = find_data_block(this_file, file_off);
        void * data_block = block_get(data_block_id, 1);
        // kprintf("~~~~~~~~~~~~~~~~~~~~~~~~~~~cpy file_off %d buff_off %d\n", (file_off) % BLOCK_SIZE, buff_off);
        memcpy(buff + buff_off, data_block + (file_off) % BLOCK_SIZE, copy_size);
        block_commit(data_block, 0);

        buff_off += copy_size;
        file_off = file_end;
    }
    this_file->offset = file_off;
    // kprintf("================%d\n", this_file->offset);
}

int seek(uint32_t fd, int offset) {
    file_t * this_file = current_running->files[fd];
    this_file->offset = offset;
}

int close(uint32_t fd) {
    file_t * this_file = current_running->files[fd];
    
    uint32_t inode_block_id = INODE_IN_BLOCK(this_file->inode_id, fs_super_block);
    uint32_t offset = this_file->inode_id % INODES_PER_BLOCK;

    inode_t * inode_block = block_get(inode_block_id, 1);
    inode_t * this_inode = inode_block + offset;

    memcpy((void*) this_inode, (void*) &this_file->inode, sizeof(inode_t));

    block_commit(inode_block, 1);

    current_running->files[fd] = 0;
    this_file->using = 0;
}

int cat(char *name) {
    int fd = open(name, O_RDONLY);
    char * this_buff = (void *) alloc_page(&page_ctrl_kernel);

    read(fd, this_buff, 100);
    for (int i = 0; i < 100; i++) {
        kprintf("%c", this_buff[i]);
    }
    free_page(&page_ctrl_kernel, (uint64_t) this_buff);
    close(fd);
}

void init_fs()
{
    read_mbr();
    
    files_init();

    disk_buff_init();
}