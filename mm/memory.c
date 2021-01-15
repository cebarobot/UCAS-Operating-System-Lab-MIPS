#include "mm.h"
#include "sched.h"
#include "string.h"

// uint64_t * global_page_table = (void*) 0xffffffffa1000000;

struct PageCtrl page_ctrl_kernel_stack;
struct PageCtrl page_ctrl_kernel;
struct PageCtrl page_ctrl_user;
struct PageCtrl page_ctrl_pgd;
struct PageCtrl page_ctrl_pt;

PGD_t * current_pgd;
PT_t * global_empty_pt;

void * zero_page;

void init_page_table()
{
    page_ctrl_init(&page_ctrl_pgd, PGD_START, 0x2000);
    page_ctrl_init(&page_ctrl_pt, PT_START, 0x2000);

    global_empty_pt = (void*) alloc_page(&page_ctrl_pt);
    memset(global_empty_pt, 0, sizeof(PT_t));
}

void init_pgd(PGD_t * pgd) {
    memset(pgd, 0, sizeof(PGD_t));
    for (int i = 0; i < NUM_ENTRY; i++) {
        pgd->entry[i] = global_empty_pt;
    }
}

void do_TLB_Refill(uint64_t addr)
{
    uint64_t pgd_offset = (addr >> 22) & 0x3ff;
    uint64_t pt_offset = (addr >> 12) & 0x3fe;
    uint64_t vpn2 = (addr >> 13);

    PT_t * pt = current_pgd->entry[pgd_offset];

    if (pt == global_empty_pt) {
        pt = (void*) alloc_page(&page_ctrl_pt);
        current_pgd->entry[pgd_offset] = pt;
        memset(pt, 0, sizeof(PT_t));
    }

    PTE_t * pte0 = &pt->entry[pt_offset];
    PTE_t * pte1 = &pt->entry[pt_offset + 1];

    if (!(*pte0 & 0x2)) {
        do_page_fault(pte0);
    }
    if (!(*pte1 & 0x2)) {
        do_page_fault(pte1);
    }

    set_cp0_entryhi(ENTRYHI(0, vpn2, current_pid));
    tlbp_operation();
    set_cp0_entrylo0(*pte0);
    set_cp0_entrylo1(*pte1);

    if (get_cp0_index() & 0x80000000) {
        tlbwr_operation();
    } else {
        tlbwi_operation();
    }
}

void TLB_Invalid_handler()
{
    uint64_t badvaddr = get_cp0_badvaddr();
    uint64_t pgd_offset = (badvaddr >> 22) & 0x3ff;
    uint64_t pt_offset = (badvaddr >> 12) & 0x3fe;
    uint64_t vpn2 = (badvaddr >> 13);

    PT_t * pt = current_pgd->entry[pgd_offset];

    if (pt == global_empty_pt) {
        pt = (void*) alloc_page(&page_ctrl_pt);
        current_pgd->entry[pgd_offset] = pt;
        memset(pt, 0, sizeof(PT_t));
    }

    PTE_t * pte0 = &pt->entry[pt_offset];
    PTE_t * pte1 = &pt->entry[pt_offset + 1];

    if (!(*pte0 & 0x2)) {
        do_page_fault(pte0);
    }
    if (!(*pte1 & 0x2)) {
        do_page_fault(pte1);
    }

    set_cp0_entryhi(ENTRYHI(0, vpn2, current_pid));
    tlbp_operation();
    set_cp0_entrylo0(*pte0);
    set_cp0_entrylo1(*pte1);

    if (get_cp0_index() & 0x80000000) {
        tlbwr_operation();
    } else {
        tlbwi_operation();
    }
}

void do_page_fault(PTE_t * pte)
{
    uint64_t phy_addr = alloc_page(&page_ctrl_user);
    uint64_t pfn = phy_addr >> 12;
    *pte = ENTRYLO(pfn, 2, 1, 1, 0);
}

void init_TLB(void)
{
    set_cp0_wired(32);
    set_cp0_pagemask(0);
    // for (int i = 0; i < 32; i++) {
    //     set_cp0_entryhi(ENTRYHI(0, i, 0));
    //     set_cp0_entrylo0(ENTRYLO(0x20000 + i*2, 2, 1, 1, 1));
    //     set_cp0_entrylo1(ENTRYLO(0x20000 + i*2 + 1, 2, 1, 1, 1));
    //     set_cp0_pagemask(0);
    //     set_cp0_index(i);
    //     tlbwi_operation();
    // }
}
void physical_frame_initial(void)
{
}

void page_ctrl_init(struct PageCtrl * page_ctrl, uint64_t start_addr, uint64_t page_size) {
    page_ctrl->start_addr = start_addr;
    page_ctrl->next_page_addr = start_addr;
    page_ctrl->page_size = page_size;
    page_ctrl->cnt_free_page = 0;
    return;
}

uint64_t alloc_page(struct PageCtrl * page_ctrl) {
    uint64_t page_addr;
    if (page_ctrl->cnt_free_page > 0) {
        page_ctrl->cnt_free_page -= 1;
        page_addr = page_ctrl->free_pages[page_ctrl->cnt_free_page];
    } else {
        page_addr = page_ctrl->next_page_addr;
        page_ctrl->next_page_addr += page_ctrl->page_size;
    }
    return page_addr;
}

void free_page(struct PageCtrl * page_ctrl, uint64_t addr) {
    addr = addr / page_ctrl->page_size * page_ctrl->page_size;
    if (page_ctrl->cnt_free_page < MAX_FREE_PAGE) {
        page_ctrl->free_pages[page_ctrl->cnt_free_page] = addr;
        page_ctrl->cnt_free_page += 1;
    }
}