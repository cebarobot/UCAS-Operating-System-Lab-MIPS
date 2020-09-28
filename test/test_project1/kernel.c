#define FUNCTION_PRINTSTR   0xffffffff8f0d5534
#define FUNCTION_PRINTCH    0xffffffff8f0d5570
#define COM_STATUS_REG      0xffffffffbfe00005
#define COM_DATA_REG        0xffffffffbfe00000

void (*printstr)(char * str) = (void *) FUNCTION_PRINTSTR;
void (*printch)(char ch) = (void *) FUNCTION_PRINTCH;
volatile char * com_status_reg = (void *) COM_STATUS_REG;
volatile char * com_data_reg = (void *) COM_DATA_REG;


void __attribute__((section(".entry_function"))) _start(void)
{
    printstr("Hello OS\r\n");
    char com_data;
    printstr("$ ");
    while (1) {
        if ((*com_status_reg) & 0x1) {
            printch(com_data = *com_data_reg);
            if (com_data == '\r') {
                printstr("\n$ ");
            }
        }
    }
}
