#include "mac.h"
#include "irq.h"
#include "mm.h"
#include "string.h"
#include "stdio.h"

#define NUM_DMA_DESC 64
queue_t recv_block_queue;
uint32_t recv_flag[PNUM] = {0};
uint32_t ch_flag = 0;
uint32_t mac_cnt = 0;

uint32_t reg_read_32(uint64_t addr)
{
    return *((uint32_t *)addr);
}
uint32_t read_register(uint64_t base, uint64_t offset)
{
    return *(volatile uint32_t *)(base + offset);
}

void reg_write_32(uint64_t addr, uint32_t data)
{
    *((uint32_t *)addr) = data;
}
uint32_t write_register(uint64_t base, uint64_t offset, uint32_t data)
{
    *(volatile uint32_t *)(base + offset) = data;
}

static void s_reset(mac_t *mac) //reset mac regs will inrupt
{
    uint32_t time = 1000000;
    reg_write_32(mac->dma_addr, 0x01);

    while ((reg_read_32(mac->dma_addr) & 0x01))
    {
        reg_write_32(mac->dma_addr, 0x01);
        while (time)
        {
            time--;
        }
    };
}

static void gmac_get_mac_addr(uint8_t *mac_addr)
{
    uint32_t addr;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0Low);
    mac_addr[0] = (addr >> 0) & 0x000000FF;
    mac_addr[1] = (addr >> 8) & 0x000000FF;
    mac_addr[2] = (addr >> 16) & 0x000000FF;
    mac_addr[3] = (addr >> 24) & 0x000000FF;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0High);
    mac_addr[4] = (addr >> 0) & 0x000000FF;
    mac_addr[5] = (addr >> 8) & 0x000000FF;
}

#if 1

/* print DMA regs */
void print_dma_regs()
{
    uint32_t regs_val1, regs_val2;

    printk(">>[DMA Register]\n");

    // [0] Bus Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaBusMode);

    printk("  [0] Bus Mode : 0x%x, ", regs_val1);

    // [3-4] RX/TX List Address Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaRxBaseAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaTxBaseAddr);
    printk("  [3-4] TX/RX : 0x%x/0x%x\n", regs_val2, regs_val1);

    // [5] Status Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaStatus);
    printk("  [5] Status : 0x%x, ", regs_val1);

    // [6] Operation Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaControl);
    printk("  [6] Control : 0x%x\n", regs_val1);

    // [7] Interrupt Enable Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaInterrupt);
    printk("  [7] Interrupt : 0x%x, ", regs_val1);

    // [8] Miss
    regs_val1 = read_register(DMA_BASE_ADDR, DmaMissedFr);
    printk("  [8] Missed : 0x%x\n", regs_val1);

    // [18-19] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrDesc);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrDesc);
    printk("  [18-19] Current Host TX/RX Description : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [20-21] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrAddr);
    printk("  [20-21] Current Host TX/RX Buffer Address : 0x%x/0x%x\n", regs_val1, regs_val2);
}

/* print DMA regs */
void print_mac_regs()
{
    printk(">>[MAC Register]\n");
    uint32_t regs_val1, regs_val2;
    uint8_t mac_addr[6];

    // [0] MAC Configure Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacConfig);
    printk("  [0] Configure : 0x%x, ", regs_val1);

    // [1] MAC Frame Filter
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFrameFilter);
    printk("  [1] Frame Filter : 0x%x\n", regs_val1);

    // [2-3] Hash Table High/Low Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacHashHigh);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacHashLow);
    printk("  [2-3] Hash Table High/Low : 0x%x-0x%x\n", regs_val1, regs_val2);

    // [6] Flow Control Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFlowControl);
    printk("  [6] Flow Control : 0x%x, ", regs_val1);

    // [8] Version Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacVersion);
    printk("  [8] Version : 0x%x\n", regs_val1);

    // [14] Interrupt Status Register and Interrupt Mask
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacInterruptStatus);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacInterruptMask);
    printk("  [14-15] Interrupt Status/Mask : 0x%x/0x%x\n", regs_val1, regs_val2);

    // MAC address
    gmac_get_mac_addr(mac_addr);
    printk("  [16-17] Mac Addr : %X:%X:%X:%X:%X:%X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

void printf_dma_regs()
{
    uint32_t regs_val1, regs_val2;

    printf(">>[DMA Register]\n");

    // [0] Bus Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaBusMode);

    printf("  [0] Bus Mode : 0x%x, ", regs_val1);

    // [3-4] RX/TX List Address Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaRxBaseAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaTxBaseAddr);
    printf("  [3-4] RX/TX : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [5] Status Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaStatus);
    printf("  [5] Status : 0x%x, ", regs_val1);

    // [6] Operation Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaControl);
    printf("  [6] Control : 0x%x\n", regs_val1);

    // [7] Interrupt Enable Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaInterrupt);
    printf("  [7] Interrupt : 0x%x, ", regs_val1);

    // [8] Miss
    regs_val1 = read_register(DMA_BASE_ADDR, DmaMissedFr);
    printf("  [8] Missed : 0x%x\n", regs_val1);

    // [18-19] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrDesc);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrDesc);
    printf("  [18-19] Current Host TX/RX Description : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [20-21] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrAddr);
    printk("  [20-21] Current Host TX/RX Buffer Address : 0x%x/0x%x\n", regs_val1, regs_val2);
}

/* print DMA regs */
void printf_mac_regs()
{
    printf(">>[MAC Register]\n");
    uint32_t regs_val1, regs_val2;
    uint8_t mac_addr[6];

    // [0] MAC Configure Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacConfig);
    printf("  [0] Configure : 0x%x, ", regs_val1);

    // [1] MAC Frame Filter
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFrameFilter);
    printf("  [1] Frame Filter : 0x%x\n", regs_val1);

    // [2-3] Hash Table High/Low Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacHashHigh);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacHashLow);
    printf("  [2-3] Hash Table High/Low : 0x%x-0x%x\n", regs_val1, regs_val2);

    // [6] Flow Control Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFlowControl);
    printf("  [6] Flow Control : 0x%x, ", regs_val1);

    // [8] Version Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacVersion);
    printf("  [8] Version : 0x%x\n", regs_val1);

    // [14] Interrupt Status Register and Interrupt Mask
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacInterruptStatus);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacInterruptMask);
    printf("  [14-15] Interrupt Status/Mask : 0x%x/0x%x\n", regs_val1, regs_val2);

    // MAC address
    gmac_get_mac_addr(mac_addr);
    printf("  [16-17] Mac Addr : %X:%X:%X:%X:%X:%X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}
#endif

void print_tx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("send buffer mac->saddr=0x%x ", mac->saddr);
    printf("mac->saddr_phy=0x%x ", mac->saddr_phy);
    printf("send discrb mac->td_phy=0x%x\n", mac->td_phy);
#if 0
    desc_t *send = (desc_t *)mac->td;
    for (i = 0; i < 1; i++)
    {
        printf("send[%d].tdes0=0x%x ", i, send[i].tdes0);
        printf("send[%d].tdes1=0x%x ", i, send[i].tdes1);
        printf("send[%d].tdes2=0x%x ", i, send[i].tdes2);
        printf("send[%d].tdes3=0x%x ", i, send[i].tdes3);
    }
#endif
}

void print_rx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("recieve buffer add mac->daddr=0x%x ", mac->daddr);
    printf("mac->daddr_phy=0x%x ", mac->daddr_phy);
    printf("recieve discrb add mac->rd_phy=0x%x\n", mac->rd_phy);
    desc_t *recieve = (desc_t *)mac->rd;
#if 0
    for(i=0;i<mac->pnum;i++)
    {
        printf("recieve[%d].tdes0=0x%x ",i,recieve[i].tdes0);
        printf("recieve[%d].tdes1=0x%x ",i,recieve[i].tdes1);
        printf("recieve[%d].tdes2=0x%x ",i,recieve[i].tdes2);
        printf("recieve[%d].tdes3=0x%x\n",i,recieve[i].tdes3);
    }
#endif
}

static uint32_t printf_recv_buffer(uint64_t recv_buffer)
{
    uint32_t i, flag, n;
    flag = 0;
    n = 0;
    for (i = 0; i < PSIZE; i++)
    {
        if ((*((uint32_t *)recv_buffer + i) != 0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f))
        {
            if (*((uint32_t *)recv_buffer + i) != 0xffffff)
            {
                printf(" %x ", *((uint32_t *)recv_buffer + i));

                if (n % 10 == 0 && n != 0)
                {
                    printf(" \n");
                }
                n++;
                flag = 1;
            }
        }
    }
    return flag;
}

/**
 * Clears all the pending interrupts.
 * If the Dma status register is read then all the interrupts gets cleared
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 */
void clear_interrupt()
{
    uint32_t data;
    data = reg_read_32(DMA_BASE_ADDR + DmaStatus);
    reg_write_32(DMA_BASE_ADDR + DmaStatus, data & 0x1ffff);
}
void mac_irq_handle(void)
{
    // TODO:
}

void irq_enable(int IRQn)
{
    // TODO:
}

void mac_recv_handle(mac_t *test_mac)
{
    // TODO:
}

static uint32_t printk_recv_buffer(uint64_t recv_buffer)
{
    uint32_t i, flag, n;
    flag = 0;
    n = 0;
    for (i = 0; i < PSIZE; i++)
    {
        if ((*((uint32_t *)recv_buffer + i) != 0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f))
        {
            if (*((uint32_t *)recv_buffer + i) != 0xffffff)
            {
                printf(" %x ", *((uint32_t *)recv_buffer + i));

                if (n % 10 == 0 && n != 0)
                {
                    printf(" \n");
                }
                n++;
                flag = 1;
            }
        }
    }
    return flag;
}

void set_sram_ctr()
{
    *((volatile uint32_t *)0xffffffffbfe10420) = 1; /* 使能GMAC0 DMA一致性 */
}

void disable_interrupt_all(mac_t *mac)
{
    reg_write_32(mac->dma_addr + DmaInterrupt, DmaIntDisable);
    return;
}

desc_t all_recv_desc[NUM_DMA_DESC];

static void mac_recv_desc_init(mac_t *mac)
{
    // TODO:

    memset(all_recv_desc, 0, sizeof(all_recv_desc));
    for (int i = 0; i < NUM_DMA_DESC; i++) {
        // all_recv_desc[i].basic.des01.erx.own = 1;
        all_recv_desc[i].basic.des01.erx.last_descriptor = 1;
        all_recv_desc[i].basic.des01.erx.first_descriptor = 1;
        all_recv_desc[i].basic.des01.erx.second_address_chained = 1;
        all_recv_desc[i].basic.des01.erx.buffer1_size = mac->psize;
        all_recv_desc[i].basic.des2 = (mac->daddr + i * mac->psize) & 0x1fffffff;
        all_recv_desc[i].basic.des3 = (uint64_t)(all_recv_desc + i + 1) & 0x1fffffff;
    }
    all_recv_desc[NUM_DMA_DESC - 1].basic.des01.erx.end_ring = 1;
    all_recv_desc[NUM_DMA_DESC - 1].basic.des3 = (uint64_t)(all_recv_desc) & 0x1fffffff;
    
    mac->rd = (uint64_t) all_recv_desc;
    mac->rd_phy = (uint64_t)(all_recv_desc) & 0x1fffffff;
}
static void mii_dul_force(mac_t *mac)
{
    reg_write_32(mac->dma_addr, 0x80);

    uint32_t conf = 0xc800;

    reg_write_32(mac->mac_addr, reg_read_32(mac->mac_addr) | (conf) | (1 << 8));
    //enable recieve all

    reg_write_32(mac->mac_addr + 0x4, 0x80000001);
}

void dma_control_init(mac_t *mac, uint32_t init_value)
{
    reg_write_32(mac->dma_addr + DmaControl, init_value);
    return;
}
void set_mac_addr(mac_t *mac, uint8_t *MacAddr)
{
    uint32_t data;

    uint32_t MacHigh = 0x40, MacLow = 0x44;
    data = (MacAddr[5] << 8) | MacAddr[4];
    reg_write_32(mac->mac_addr + MacHigh, data);
    data = (MacAddr[3] << 24) | (MacAddr[2] << 16) | (MacAddr[1] << 8) | MacAddr[0];
    reg_write_32(mac->mac_addr + MacLow, data);
}

desc_t all_send_desc[NUM_DMA_DESC];

static void mac_send_desc_init(mac_t *mac)
{
    // TODO:

    memset(all_send_desc, 0, sizeof(all_send_desc));
    for (int i = 0; i < NUM_DMA_DESC; i++) {
        // all_send_desc[i].basic.des01.etx.own = 1;
        all_send_desc[i].basic.des01.etx.last_segment = 1;
        all_send_desc[i].basic.des01.etx.first_segment = 1;
        all_send_desc[i].basic.des01.etx.checksum_insertion = 1;
        all_send_desc[i].basic.des01.etx.second_address_chained = 1;
        all_send_desc[i].basic.des01.etx.buffer1_size = mac->psize;
        all_send_desc[i].basic.des2 = mac->saddr & 0x1fffffff;
        all_send_desc[i].basic.des3 = (uint64_t)(all_send_desc + i + 1) & 0x1fffffff;
    }
    all_send_desc[NUM_DMA_DESC - 1].basic.des01.etx.end_ring = 1;
    all_send_desc[NUM_DMA_DESC - 1].basic.des3 = (uint64_t)(all_send_desc) & 0x1fffffff;

    mac->td = (uint64_t)all_send_desc;
    mac->td_phy = (uint64_t)(all_send_desc) & 0x1fffffff;
}

/* buf_addr is the total recv buffer's address; size means the total size of recv buffer;
the total recv buffer may have some little recv buffer;
num means recv buffer's num ;length is each small recv buffer's size*/
uint32_t do_net_recv(uint64_t buf_addr, uint64_t size, uint64_t num, uint64_t length)
{

    mac_t mac;

    mac.mac_addr = GMAC_BASE_ADDR;
    mac.dma_addr = DMA_BASE_ADDR;

    mac.psize = PSIZE * 4; // 128bytes
    mac.pnum = PNUM;       // pnum
    mac.daddr = buf_addr;
    mac_recv_desc_init(&mac);
    dma_control_init(&mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&mac);

    mii_dul_force(&mac);

    reg_write_32(GMAC_BASE_ADDR, reg_read_32(GMAC_BASE_ADDR) | 0x4);
    write_register(DMA_BASE_ADDR, DmaRxBaseAddr, mac.rd_phy);
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02200002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));
    reg_write_32(DMA_BASE_ADDR + 0x1c, DMA_INTR_DEFAULT_MASK);

    // TODO: YOU NEED ADD RECV CODE
    desc_t * recv_desc = (void*)mac.rd;

    for (int i = 0; i < num; i++) {
        recv_desc[i].basic.des01.erx.own = 1;
    }
    for (int i = 0; i < num; i++) {
        write_register(DMA_BASE_ADDR, DmaRxPollDemand, 1);
    }
    for (int i = 0; i < num; i++) {
        while (recv_desc[i].basic.des01.erx.own);
        sys_move_cursor(1, 2);
        printf("==========%d=========\n", i);
        printk_recv_buffer(mac.daddr + mac.psize * i);
    }
    return 0;
}

/* buf_addr is sended buffer's address; size means sended buffer's size;
num means sended buffer's times*/
void do_net_send(uint64_t buf_addr, uint64_t size, uint64_t num)
{
    uint64_t td_phy;
    mac_t mac;
    mac.mac_addr = GMAC_BASE_ADDR;
    mac.dma_addr = DMA_BASE_ADDR;

    mac.psize = PSIZE * 4;
    mac.pnum = PNUM;
    td_phy = mac.td_phy;
    mac.saddr = buf_addr;
    mac_send_desc_init(&mac);
    dma_control_init(&mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&mac);

    mii_dul_force(&mac);

    reg_write_32(GMAC_BASE_ADDR, reg_read_32(GMAC_BASE_ADDR) | 0x8);                    // enable MAC-TX
    reg_write_32(DMA_BASE_ADDR + 0x10, mac.td_phy);
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02202000); //0x02202002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));
    reg_write_32(DMA_BASE_ADDR + 0x1c, DMA_INTR_DEFAULT_MASK);

    // TODO: YOU NEED ADD SEND CODE
    desc_t * send_desc = (void*)mac.td;

    for (int i = 0; i < num; i++) {
        send_desc[i].basic.des01.etx.own = 1;
    }
    for (int i = 0; i < num; i++) {
        write_register(DMA_BASE_ADDR, DmaTxPollDemand, 1);
    }

}
void set_mac_int()
{
    volatile uint8_t *entry_gmac0;
    entry_gmac0 = (void *)(0xffffffff1fe1140c | 0xa0000000);
    *entry_gmac0 = 0x41; //0 core ip6 int2
}
void do_init_mac(void)
{
    mac_t test_mac;
    uint32_t i;

    test_mac.mac_addr = GMAC_BASE_ADDR;
    test_mac.dma_addr = DMA_BASE_ADDR;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum
    uint8_t mac_addr[6];

    gmac_get_mac_addr(mac_addr);
    set_sram_ctr(); /* 使能GMAC0 */
                    //   s_reset(&test_mac); //will interrupt
    disable_interrupt_all(&test_mac);
    set_mac_addr(&test_mac, mac_addr);
    // register_irq_handler(12, mac_irq_handle);
    reg_write_32(GMAC_BASE_ADDR + 0X3C, 1);
    set_mac_int();
}

void do_wait_recv_package(void)
{
    // TODO:
}
