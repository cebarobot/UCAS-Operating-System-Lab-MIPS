#include "mm.h"
#include "string.h"

uint64_t * global_page_table = (void*) 0xffffffffa1000000;
static int cur_pfn = 0x20000;

struct PageCtrl page_ctrl_kernal;
struct PageCtrl page_ctrl_user;

void init_page_table()
{
    set_cp0_context((uint64_t)global_page_table);
    memset(global_page_table, 0, 0x80000 * 8);
    // for (int i = 0; i < 0x80000; i++) {
    //     global_page_table[i] = ENTRYLO(0, 0, 0, 0, 0);
    // }
}
void do_TLB_Refill(uint64_t addr)
{
    uint64_t * page_table = global_page_table;
    uint64_t vpn2 = addr >> 13;
    uint64_t vpn0 = vpn2 << 1;
    uint64_t vpn1 = vpn2 << 1 + 1;
    if (!(page_table[vpn0] & 0x2)) {
        do_page_fault(page_table, vpn0);
    }
    if (!(page_table[vpn1] & 0x2)) {
        do_page_fault(page_table, vpn1);
    }
    set_cp0_entryhi(ENTRYHI(0, vpn2, 0));
    tlbp_operation();
    set_cp0_entrylo0(page_table[vpn0]);
    set_cp0_entrylo1(page_table[vpn1]);
    set_cp0_pagemask(0);
    if (get_cp0_index() & 0x80000000) {
        tlbwr_operation();
    } else {
        tlbwi_operation();
    }
}

void TLB_Invalid_handler()
{
    uint64_t * page_table = global_page_table;
    uint64_t vpn2 = get_cp0_entryhi() >> 13;
    uint64_t vpn0 = vpn2 << 1;
    uint64_t vpn1 = vpn2 << 1 + 1;
    if (!(page_table[vpn0] & 0x2)) {
        do_page_fault(page_table, vpn0);
    }
    if (!(page_table[vpn1] & 0x2)) {
        do_page_fault(page_table, vpn1);
    }
    set_cp0_entryhi(ENTRYHI(0, vpn2, 0));
    tlbp_operation();
    set_cp0_entrylo0(page_table[vpn0]);
    set_cp0_entrylo1(page_table[vpn1]);
    set_cp0_pagemask(0);
    if (get_cp0_index() & 0x80000000) {
        tlbwr_operation();
    } else {
        tlbwi_operation();
    }
}

void do_page_fault(uint64_t * page_table, uint64_t vpn)
{
    page_table[vpn] = ENTRYLO(cur_pfn, 2, 1, 1, 1);
    cur_pfn += 1;
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

void page_ctrl_init(struct PageCtrl * page_ctrl, uint32_t start_addr) {
    page_ctrl->start_addr = start_addr;
    page_ctrl->next_page_addr = start_addr;
    page_ctrl->cnt_free_page = 0;
    return;
}

uint32_t alloc_page(struct PageCtrl * page_ctrl) {
    uint32_t page_addr;
    if (page_ctrl->cnt_free_page > 0) {
        page_ctrl->cnt_free_page -= 1;
        page_addr = page_ctrl->free_pages[page_ctrl->cnt_free_page];
    } else {
        page_addr = page_ctrl->next_page_addr;
        page_ctrl->next_page_addr += PAGE_SIZE;
    }
    return page_addr;
}

void free_page(struct PageCtrl * page_ctrl, uint32_t addr) {
    addr = addr / PAGE_SIZE * PAGE_SIZE;
    if (page_ctrl->cnt_free_page < MAX_FREE_PAGE) {
        page_ctrl->free_pages[page_ctrl->cnt_free_page] = addr;
        page_ctrl->cnt_free_page += 1;
    }
}