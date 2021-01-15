#include "fs.h"
#include "string.h"
#include "common.h"
#include "mm.h"
#include "bitmap.h"
#include "stdio.h"

char cur_dir_name[64] = {"/"};
int cur_dir_inode;

MBR_t disk_mbr;
DPT_entry_t disk_partition_table[4];
int mbr_modified = 0;

int current_partition;
int fs_mounted = 0;
super_block_t * fs_super_block;
void * fs_inode_bitmap;

int fs_super_block_modified = 0;
int fs_inode_bitmap_modified = 0;

void read_mbr() {
    sd_card_read(&disk_mbr, 0, SECTOR_SIZE);
    mbr_modified = 0;
    if (disk_mbr.boot_signature != MBR_BOOT_SIG) {
        mbr_modified = 1;
        disk_mbr.boot_signature = MBR_BOOT_SIG;
    }
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

int mount_file_system(int part_id) {
    if (fs_mounted) {
        kprintf("error: mounted\n");
        return -1;
    }
    fs_super_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fs_super_block, part_id, 1);
    if (fs_super_block->magic_num != KFS_MAGIC) {
        kprintf("%08x\n", fs_super_block->magic_num);
        free_page(&page_ctrl_kernel, (uint64_t) fs_super_block);
        kprintf("error: wrong magic\n");
        return -1;
    }
    fs_inode_bitmap = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fs_inode_bitmap, part_id, fs_super_block->inode_bitmap_start);
    current_partition = part_id;
    fs_mounted = 1;
    kprintf("mount done\n");
    return 0;
}

int unmount_file_system() {
    if (!fs_mounted) {
        return -1;
    }
    block_write(fs_inode_bitmap, current_partition, fs_super_block->inode_bitmap_start);
    block_write(fs_super_block, current_partition, 1);
    free_page(&page_ctrl_kernel, (uint64_t) fs_inode_bitmap);
    free_page(&page_ctrl_kernel, (uint64_t) fs_super_block);
    fs_mounted = 0;
    return 0;
}

int init_file_system(int part_id) {
    if (fs_mounted) {
        return -1;
    }
    void * buff = (void *) alloc_page(&page_ctrl_kernel);
    // setup super block
    kprintf("setup super block\n");
    fs_super_block = (void *) alloc_page(&page_ctrl_kernel);
    fs_super_block->magic_num = KFS_MAGIC;
    fs_super_block->size = disk_partition_table[part_id].sector_num / SECTOR_PER_BLOCK;
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
    fs_inode_bitmap = (void *) alloc_page(&page_ctrl_kernel);
    bitmap_init_byte(fs_inode_bitmap, BLOCK_SIZE);
    bitmap_set(fs_inode_bitmap, 0);
    // write inode map
    kprintf("write inode map\n");
    block_write(fs_inode_bitmap, current_partition, fs_super_block->inode_bitmap_start);
    fs_inode_bitmap_modified = 0;

    // setup block map
    kprintf("setup block map\n");
    bitmap_init_byte(buff, BLOCK_SIZE);
    for (int i = 0; i <= fs_super_block->data_start; i++) {         // all block before data block and data block
        bitmap_set(buff, i);
    }
    // write block map
    kprintf("write block map\n");
    block_write(buff, current_partition, fs_super_block->block_bitmap_start);
    for (int i = 1; i < fs_super_block->size_block_bitmap; i++) {
        block_clear(current_partition, fs_super_block->block_bitmap_start + i);
    }

    // setup inode
    kprintf("setup inode\n");
    memset(buff, 0, BLOCK_SIZE);
    inode_t * inode_block = buff;
    inode_block[0].type = TYPE_DIR;
    inode_block[0].n_links = 1;
    inode_block[0].size = 1;
    inode_block[0].addrs[0] = fs_super_block->data_start;
    // write inode
    kprintf("write inode\n");
    block_write(buff, current_partition, fs_super_block->inode_start);

    // setup data block
    kprintf("setup data block\n");
    memset(buff, 0, BLOCK_SIZE);
    dir_entry_t * root_dir = buff;
    strcpy(root_dir[0].name, ".");
    root_dir[0].inode_id = 0;
    strcpy(root_dir[1].name, "..");
    root_dir[1].inode_id = 0;
    block_write(buff, current_partition, fs_super_block->data_start);

    free_page(&page_ctrl_kernel, (uint64_t) buff);
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
    int inode_id;
    for (int i = 0; i < BITS_PER_BLOCK; i++) {
        if (bitmap_check(fs_inode_bitmap, i) == 0) {
            inode_id = i;
            break;
        }
    }
    bitmap_set(fs_inode_bitmap, inode_id);
    block_write(fs_inode_bitmap, current_partition, fs_super_block->inode_bitmap_start);
    return inode_id;
}

void free_inode(uint32_t inode_id) {
    bitmap_clear(fs_inode_bitmap, inode_id);
    block_write(fs_inode_bitmap, current_partition, fs_super_block->inode_bitmap_start);
}

uint32_t alloc_data_block() {
    void * block_bitmap = (void *) alloc_page(&page_ctrl_kernel);
    int bitmap_block_id = fs_super_block->block_bitmap_start;
    block_read(block_bitmap, current_partition, bitmap_block_id);

    int data_block_id;

    for (int i = fs_super_block->data_start; i < fs_super_block->size; i++) {
        if (i / BITS_PER_BLOCK + fs_super_block->block_bitmap_start != bitmap_block_id) {
            bitmap_block_id += 1;
            block_read(block_bitmap, current_partition, bitmap_block_id);
        }
        int offset = i % BITS_PER_BLOCK;
        if (bitmap_check(block_bitmap, offset) == 0) {
            bitmap_set(block_bitmap, offset);
            data_block_id = i;
            break;
        }
    }

    block_write(block_bitmap, current_partition, bitmap_block_id);
    free_page(&page_ctrl_kernel, (uint64_t) block_bitmap);

    return data_block_id;
}

void free_data_block(uint32_t data_block_id) {
    void * block_bitmap = (void *) alloc_page(&page_ctrl_kernel);

    int bitmap_block_id = data_block_id / BITS_PER_BLOCK + fs_super_block->block_bitmap_start;
    int offset = data_block_id % BITS_PER_BLOCK;

    block_read(block_bitmap, current_partition, bitmap_block_id);

    bitmap_clear(block_bitmap, offset);

    block_write(block_bitmap, current_partition, bitmap_block_id);
    
    free_page(&page_ctrl_kernel, (uint64_t) block_bitmap);
}

void do_ls(uint32_t inode_id) {
    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;
    
    inode_t * inode_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(inode_block, current_partition, inode_block_id);

    inode_t * this_inode = inode_block + offset;

    dir_entry_t * dir_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(dir_block, current_partition, this_inode->addrs[0]);

    for (int i = 0; i < DENTRY_PER_BLOCK; i++) {
        if (dir_block[i].name[0] == 0) {
            break;
        }
        kprintf("%s\n", dir_block[i].name);
    }

    free_page(&page_ctrl_kernel, (uint64_t) inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) dir_block);
}

void do_mkdir(uint32_t fa_inode_id, char * name) {
    uint32_t fa_inode_block_id = INODE_IN_BLOCK(fa_inode_id, fs_super_block);
    uint32_t fa_offset = fa_inode_id % INODES_PER_BLOCK;

    inode_t * fa_inode_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fa_inode_block, current_partition, fa_inode_block_id);

    inode_t * fa_inode = fa_inode_block + fa_offset;

    dir_entry_t * fa_dir_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fa_dir_block, current_partition, fa_inode->addrs[0]);

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
    block_write(fa_dir_block, current_partition, fa_inode->addrs[0]);

    free_page(&page_ctrl_kernel, (uint64_t) fa_inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) fa_dir_block);
    
    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;
    
    inode_t * inode_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(inode_block, current_partition, inode_block_id);

    inode_t * this_inode = inode_block + offset;
    
    this_inode->type = TYPE_DIR;
    this_inode->n_links = 1;
    this_inode->size = 1;
    this_inode->addrs[0] = alloc_data_block();

    block_write(inode_block, current_partition, inode_block_id);

    dir_entry_t * dir_block = (void *) alloc_page(&page_ctrl_kernel);
    memset(dir_block, 0, BLOCK_SIZE);

    strcpy(dir_block[0].name, ".");
    dir_block[0].inode_id = inode_id;
    strcpy(dir_block[1].name, "..");
    dir_block[1].inode_id = fa_inode_id;
    
    block_write(dir_block, current_partition, this_inode->addrs[0]);

    free_page(&page_ctrl_kernel, (uint64_t) inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) dir_block);
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

    inode_t * inode_block = (void *) alloc_page(&page_ctrl_kernel);
    dir_entry_t * dir_block = (void *) alloc_page(&page_ctrl_kernel);

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
        
        block_read(inode_block, current_partition, inode_block_id);

        this_inode = inode_block + offset;

        if (this_inode->type != TYPE_DIR) {
            kprintf("ERROR: %s is not dir\n", ppp);
            error = 1;
            break;
        }

        block_read(dir_block, current_partition, this_inode->addrs[0]);

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
    }
    free_page(&page_ctrl_kernel, (uint64_t) inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) dir_block);

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

    inode_t * inode_block = (void *) alloc_page(&page_ctrl_kernel);
    dir_entry_t * dir_block = (void *) alloc_page(&page_ctrl_kernel);

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
        
        block_read(inode_block, current_partition, inode_block_id);

        this_inode = inode_block + offset;

        if (this_inode->type != TYPE_DIR) {
            kprintf("ERROR: %s is not dir\n", ppp);
            error = 1;
            break;
        }

        block_read(dir_block, current_partition, this_inode->addrs[0]);

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
    }

    inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    offset = inode_id % INODES_PER_BLOCK;
    
    block_read(inode_block, current_partition, inode_block_id);

    this_inode = inode_block + offset;

    if (this_inode->type != TYPE_DIR) {
        kprintf("ERROR: %s is not dir\n", ppp);
        error = 1;
    }
    
    free_page(&page_ctrl_kernel, (uint64_t) inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) dir_block);

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

    inode_t * inode_block = (void *) alloc_page(&page_ctrl_kernel);
    dir_entry_t * dir_block = (void *) alloc_page(&page_ctrl_kernel);

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
        
        block_read(inode_block, current_partition, inode_block_id);

        this_inode = inode_block + offset;

        if (this_inode->type != TYPE_DIR) {
            kprintf("ERROR: %s is not dir\n", ppp);
            error = 1;
            break;
        }

        block_read(dir_block, current_partition, this_inode->addrs[0]);

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
    }

    inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    offset = inode_id % INODES_PER_BLOCK;
    
    block_read(inode_block, current_partition, inode_block_id);

    this_inode = inode_block + offset;

    if (this_inode->type != TYPE_DIR) {
        kprintf("ERROR: %s is not dir\n", ppp);
        error = 1;
    }
    
    free_page(&page_ctrl_kernel, (uint64_t) inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) dir_block);

    if (error == 0) {
        if (inode_id) {
            strcpy(cur_dir_name, dir_name);
        } else {
            strcpy(cur_dir_name, "/");
        }
        cur_dir_inode = inode_id;
    }
}

void do_rmdir(uint32_t inode_id) {
    inode_t * inode_block = (void *) alloc_page(&page_ctrl_kernel);
    dir_entry_t * dir_block = (void *) alloc_page(&page_ctrl_kernel);

    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;
    
    block_read(inode_block, current_partition, inode_block_id);

    inode_t * this_inode = inode_block + offset;

    block_read(dir_block, current_partition, this_inode->addrs[0]);

    uint32_t fa_inode_id = dir_block[1].inode_id;
    
    uint32_t fa_inode_block_id = INODE_IN_BLOCK(fa_inode_id, fs_super_block);
    uint32_t fa_offset = fa_inode_id % INODES_PER_BLOCK;

    inode_t * fa_inode_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fa_inode_block, current_partition, fa_inode_block_id);

    inode_t * fa_inode = fa_inode_block + fa_offset;

    dir_entry_t * fa_dir_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fa_dir_block, current_partition, fa_inode->addrs[0]);

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

    block_write(fa_dir_block, current_partition, fa_inode->addrs[0]);
    
    free_inode(inode_id);
    free_data_block(inode_block->addrs[0]);
    
    free_page(&page_ctrl_kernel, (uint64_t) fa_inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) fa_dir_block);

    free_page(&page_ctrl_kernel, (uint64_t) inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) dir_block);
}

void do_touch(uint32_t fa_inode_id, char * name) {
    uint32_t fa_inode_block_id = INODE_IN_BLOCK(fa_inode_id, fs_super_block);
    uint32_t fa_offset = fa_inode_id % INODES_PER_BLOCK;

    inode_t * fa_inode_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fa_inode_block, current_partition, fa_inode_block_id);

    inode_t * fa_inode = fa_inode_block + fa_offset;

    dir_entry_t * fa_dir_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(fa_dir_block, current_partition, fa_inode->addrs[0]);

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
    block_write(fa_dir_block, current_partition, fa_inode->addrs[0]);

    free_page(&page_ctrl_kernel, (uint64_t) fa_inode_block);
    free_page(&page_ctrl_kernel, (uint64_t) fa_dir_block);
    
    uint32_t inode_block_id = INODE_IN_BLOCK(inode_id, fs_super_block);
    uint32_t offset = inode_id % INODES_PER_BLOCK;
    
    inode_t * inode_block = (void *) alloc_page(&page_ctrl_kernel);
    block_read(inode_block, current_partition, inode_block_id);

    inode_t * this_inode = inode_block + offset;
    
    this_inode->type = TYPE_FILE;
    this_inode->n_links = 1;
    this_inode->size = 0;

    block_write(inode_block, current_partition, inode_block_id);

    free_page(&page_ctrl_kernel, (uint64_t) inode_block);
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
 
void init_fs()
{
    read_mbr();
}
