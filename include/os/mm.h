#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_
#include "type.h"
#include "sched.h"

#define PAGE_SIZE 0x1000

typedef struct PTE
{

} PTE_t; /* 128 + 28 = 156B */

#define ENTRYHI(r, vpn2, asid) (uint64_t) \
    ( (((r) & (uint64_t)0x3) << 62) | (((vpn2) & 0x7FFFFFF) << 13) | (asid & 0xff) )
#define ENTRYLO(pfn, c, d, v, g) (uint64_t) \
    ( (((pfn) & 0xFFFFFF) << 6) | (((c) & 0x7) << 3) | (((d) & 0x1) << 2) | (((v) & 0x1) << 1) |((g) & 0x1) )

void init_page_table();
void do_TLB_Refill(uint64_t);

void TLB_Invalid_handler();
void do_page_fault(uint64_t * page_table, uint64_t vpn);

void init_TLB(void);
void physical_frame_initial(void);

void set_cp0_entryhi(uint64_t);
uint64_t get_cp0_entryhi();
uint64_t get_cp0_index();
void set_cp0_index(uint64_t);
uint64_t get_cp0_badvaddr();
uint64_t get_cp0_entrylo0();
void set_cp0_entrylo0(uint64_t);
uint64_t get_cp0_entrylo1();
void set_cp0_entrylo1(uint64_t);
void set_cp0_pagemask(uint64_t);
void tlbwr_operation();
void tlbwi_operation();
void tlbp_operation();
void set_cp0_wired(uint64_t);
void set_cp0_context(uint64_t);

/**
 * Allocation of page
 */

#define MAX_FREE_PAGE 125

/** 
 * struct of page allocation control infomation
 */
struct PageCtrl {
    uint32_t start_addr;
    uint32_t next_page_addr;
    uint32_t cnt_free_page;
    uint32_t free_pages[MAX_FREE_PAGE];
};

extern struct PageCtrl page_ctrl_kernal;
extern struct PageCtrl page_ctrl_user;

/**
 * Initialize page allocation contraller
 * @param page_ctrl pointer to stuct PageAllocCtl
 * @param start_addr start address of memory space
 * @param page_size size of one page
 */
void page_ctrl_init(struct PageCtrl * page_ctrl, uint32_t start_addr);

/**
 * Alloc a page
 * @param page_ctrl pointer to struct PageAllocCtl
 * @return address of page
 */
uint32_t alloc_page(struct PageCtrl * page_ctrl);

/**
 * Free a page
 * @param page_ctrl pointer to struct PageAllocCtl
 * @param addr address of page
 */
void free_page(struct PageCtrl * page_ctrl, uint32_t addr);


#endif
