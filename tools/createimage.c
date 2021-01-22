#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <getopt.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 0x200
#define OS_SIZE_LOC 0x1fe
#define DPT_LOC 0x1be
#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define BOOT_LOADER_SIG_1 0x55
#define BOOT_LOADER_SIG_2 0xaa
#define BOOT_MEM_LOC 0x7c00
#define OS_MEM_LOC 0x1000
#define PARTITION_TYPE 0x1A

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

// struct of store cli options
static const struct option long_option[] = {
    {"extended", no_argument, &options.extended, 1},
    {"vm", no_argument, &options.vm, 1},
    {NULL, no_argument, NULL, 0}
};

typedef struct DPT_entry {
    uint32_t status     :8;
    uint32_t start_chs  :24;
    uint32_t type       :8;
    uint32_t end_chs    :24;
    uint32_t start_lba;
    uint32_t sector_num;
} DPT_entry_t;

DPT_entry_t dpt[4];

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph, Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE *fp, FILE *img, int *nbytes, int *first);
static void write_os_size(int nbytes, FILE *img);
static void write_dpt(FILE *img);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE *fp, FILE *img, int *nbytes, int *first);

int main(int argc, char** argv) {
    while (getopt_long(argc,argv,"as",long_option,NULL) != -1);
    create_image(argc - optind, argv + optind);
    
    return 0;
}

static void create_image(int nfiles, char *files[]) {
    FILE * image_file = fopen(IMAGE_FILE, "wb");
    Elf64_Ehdr file_ehdr;
    Elf64_Phdr file_phdr;
    int nbytes = 0, first = 1;

    for (int i = 0; i < nfiles; i++) {
        FILE * fp = fopen(files[i], "rb");
        read_ehdr(&file_ehdr, fp);
        printf("0x%04lx: %s\n", file_ehdr.e_entry, files[i]);

        for (int ph = 0; ph < file_ehdr.e_phnum; ph++) {
            if (options.extended) {
                printf("\tsegment %d\n", ph);
            }
            read_phdr(&file_phdr, fp, ph, file_ehdr);
            if (options.extended) {
                printf("\t\toffset 0x%04lx", file_phdr.p_offset);
                printf("\t\tvaddr 0x%04lx\n", file_phdr.p_vaddr);
                printf("\t\tfilesz 0x%04lx", file_phdr.p_filesz);
                printf("\t\tmemsz 0x%04lx\n", file_phdr.p_memsz);
            }

            write_segment(file_ehdr, file_phdr, fp, image_file, &nbytes, &first);

        }

        int padding = SECTOR_SIZE - nbytes % SECTOR_SIZE;
        nbytes += padding;
        for (int i = 0; i < padding; i++) {
            fputc(0, image_file);
        }
        fseek(image_file, nbytes, SEEK_SET);
        printf("\t\tpadding up to 0x%04x\n", nbytes);

        fclose(fp);
    }

    write_os_size(nbytes - SECTOR_SIZE, image_file);
    write_dpt(image_file);

    fclose(image_file);
}

static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp) {
    fseek(fp, 0, SEEK_SET);
    int nbytes = fread(ehdr, 1, sizeof(Elf64_Ehdr), fp);
    if (nbytes != sizeof(Elf64_Ehdr)) {
        error("ERROR: Cannot read ELF header.\n");
    }
}

static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph, Elf64_Ehdr ehdr) {
    if (ph >= ehdr.e_phnum) {
        error("ERROR: Segment %d not found, only have %d segments.\n", ph, ehdr.e_phnum);
    }
    fseek(fp, ehdr.e_phoff, SEEK_SET);
    fseek(fp, ph * ehdr.e_phentsize, SEEK_CUR);
    int nbytes = fread(phdr, 1, sizeof(Elf64_Phdr), fp);
    if (nbytes != sizeof(Elf64_Phdr)) {
        error("ERROR: Cannot read program header.\n");
    }
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE *fp, FILE *img, int *total_nbytes, int *first) {
    fseek(fp, phdr.p_offset, SEEK_SET);
    fseek(img, *total_nbytes, SEEK_SET);

    unsigned char * buff = malloc(phdr.p_memsz);
    memset(buff, 0, phdr.p_memsz);

    int nbytes = fread(buff, 1, phdr.p_filesz, fp);
    if (nbytes != phdr.p_filesz) {
        error("ERROR: Cannot read segment\n");
    }

    nbytes = fwrite(buff, 1, phdr.p_memsz, img);
    if (nbytes != phdr.p_memsz) {
        error("ERROR: Cannot write segment\n");
    }

    *total_nbytes += nbytes;
    free(buff);

    if (options.extended) {
        printf("\t\twriting 0x%04x bytes\n", nbytes);
    }
}

static void write_dpt(FILE *img) {
    dpt[0].status = 0x80;
    dpt[0].start_chs = 0xFFFFFE;
    dpt[0].type = PARTITION_TYPE;
    dpt[0].end_chs = 0xFFFFFE;
    dpt[0].start_lba = 0x1000;
    dpt[0].sector_num = 0x10000;
    printf("writing dpt...\n");
    fseek(img, DPT_LOC, SEEK_SET);
    fwrite(dpt, sizeof(struct DPT_entry), 4, img);
}
 
static void write_os_size(int nbytes, FILE *img) {
    uint16_t os_size = nbytes / 0x200;
    printf("os_size: %d sectors\n", os_size);
    fseek(img, OS_SIZE_LOC, SEEK_SET);
    fwrite(&os_size, sizeof(uint16_t), 1, img);
}

/* print an error message and exit */
static void error(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
