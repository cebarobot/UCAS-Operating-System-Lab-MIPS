#include "mm.h"

void init_page_table()
{
}
void do_TLB_Refill()
{
}

void do_page_fault()
{
}

void init_TLB(void)
{
    set_cp0_pagemask(0);
    for (int i = 0; i < 32; i++) {
        set_cp0_entryhi(ENTRYHI(0, i, 0));
        set_cp0_entrylo0(ENTRYLO(0x20000 + i*2, 2, 1, 1, 1));
        set_cp0_entrylo1(ENTRYLO(0x20000 + i*2 + 1, 2, 1, 1, 1));
        set_cp0_pagemask(0);
        set_cp0_index(i);
        tlbwi_operation();
    }
}
void physical_frame_initial(void)
{
}