/* compile in macos:
 * gcc gd32up.c libserialport.a -o gd32up -framework IOKit -framework CoreFoundation */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "libserialport.h"

#define MAX_WAIT     600
#define BLK_SIZE     0x100

void print_hex(const char *name, const char *buf, size_t count)
{
    int i;
    printf("%s %zu: ", name, count);
    for (i = 0; i < count; i++)
        printf("%02X ", (unsigned char)buf[i]);
    printf("\n");
}

int sp_write(struct sp_port *port, const void *buf, size_t count)
{
    int wbyte;
    wbyte = sp_blocking_write(port, buf, count, MAX_WAIT);
    
//    print_hex("wr", buf, wbyte);
    return wbyte;
}

int sp_read(struct sp_port *port, void *buf, size_t count)
{
    int rbyte;
    rbyte = sp_blocking_read(port, buf, count, MAX_WAIT);
    
//    print_hex("rd", buf, rbyte);
    return rbyte;
}

void print_serial_list()
{
    struct sp_port **ports;
    int i;

    printf("current valid serial port(s):\n");
    sp_list_ports(&ports);
    for (i = 0; ports[i]; i++)
        printf("%s\n", sp_get_port_name(ports[i]));
    sp_free_port_list(ports);
}

struct sp_port * gd32_init_serial(const char *name)
{
    struct sp_port *port;
    int baudrate = 115200;

    if (SP_OK != sp_get_port_by_name(name, &port))
        return NULL;

    if (SP_OK != sp_open(port, SP_MODE_READ_WRITE))
        return NULL;

    // clear input/output buffer.
    sp_flush(port, SP_BUF_BOTH);

    // gd32f150 supported protocol 115200, 8e1.
    printf("set bandrate to %d.\n", baudrate);
    sp_set_baudrate(port, baudrate);
    sp_set_bits(port, 8);
    sp_set_parity(port, SP_PARITY_EVEN);
    sp_set_stopbits(port, 1);
    
    // necessary, or system will drop 0x11 and 0x13.
    sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

    return port;
}

void gd32_uninit_serial(struct sp_port *port)
{
    sp_close(port);
    sp_free_port(port);
}

char block_xor(const char *d, int size)
{
    char out = 0;
    while (size)
        out ^= d[--size];
    return out;
}

int gd32_init_bootloader(struct sp_port *port)
{
    char buf[1];
    int i;

    // set chip baudrate.
    for (i = 0; i <= 5; i++) {
        buf[0] = 0x7f;
        sp_write(port, buf, 1);
        if (sp_read(port, buf, 1) > 0) {
            if (buf[0] == 0x1f)
                // NOTE: weird issue, if I get first 0x79, next command will fail.
                // so I send 0x7f again, once get 0x1f, everything normal.
                break;       // valid result.

            // we have to try again...
        }
    }
    if (i > 5)
        return -__LINE__;

    sleep(1);
    return 1;
}

int gd32_erase_flash(struct sp_port *port)
{
    char buf[2];

    // erase memory command is 0x43.
    buf[0] = 0x43;
    buf[1] = ~buf[0];
    sp_write(port, buf, 2);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return -__LINE__;
    
    printf("erase flash...");
   
    // requests to erase all blocks.
    buf[0] = 0xff;
    buf[1] = ~buf[0];
    sp_write(port, buf, 2);
    // erase takes around 200ms, MAX_WAIT must big enough.
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return -__LINE__;

    printf("done\n");
    return 1;
}

int gd32_read_memory(struct sp_port *port, int addr, char *d, int size)
{
    char buf[5];
    int used;

    // read memory command is 0x11.
    buf[0] = 0x11;
    buf[1] = ~buf[0];
    sp_write(port, buf, 2);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return -__LINE__;

    // send address to remote.
    buf[0] = (addr >> 24) & 0xff;
    buf[1] = (addr >> 16) & 0xff;
    buf[2] = (addr >> 8) & 0xff;
    buf[3] = addr & 0xff;
    buf[4] = block_xor(buf, 4);
    sp_write(port, buf, 5);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return -__LINE__;

    // send request read byte size.
    buf[0] = (size - 1) & 0xff;
    buf[1] = ~buf[0];
    sp_write(port, buf, 2);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return -__LINE__;

    // read real data from serial port.
    used = sp_read(port, d, size);
    if (size != used)
        return -used;

    return size;
}

int gd32_write_memory(struct sp_port *port, int addr, char *d, int size)
{
    char buf[BLK_SIZE + 2];

    // write memory command is 0x31.
    buf[0] = 0x31;
    buf[1] = ~buf[0];
    sp_write(port, buf, 2);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return -__LINE__;

    // send address to remote.
    buf[0] = (addr >> 24) & 0xff;
    buf[1] = (addr >> 16) & 0xff;
    buf[2] = (addr >> 8) & 0xff;
    buf[3] = addr & 0xff;
    buf[4] = block_xor(buf, 4);
    sp_write(port, buf, 5);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return -__LINE__;

    // send write size, data, and xor.
    buf[0] = (size - 1) & 0xff;
    memcpy(buf + 1, d, size);
    buf[size + 1] = block_xor(buf, size + 1);
    sp_write(port, buf, size + 2);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return -__LINE__;

    // we already have xor check, no need more compare.
    return size;
}

const char *gd32_get_unique_id(struct sp_port *port)
{
    static char id[25] = "";
    unsigned char buf[12] = {0};
    char *p = id, i;

    if (gd32_read_memory(port, 0x1ffff7ac, buf, 12) < 0)
        return NULL;

    for (i = 0; i < 12; i++)
        p += sprintf(p, "%02X", buf[i]);

    return id;
}

void gd32_read_flash_to_file(const char *name, const char *path)
{
    struct sp_port *port;

    FILE *fp;
    int i;
    time_t ct = time(NULL);
    const char *id = NULL;

    port = gd32_init_serial(name);
    if (port == NULL) {
        printf("can not open serial %s.\n", name);
        return;     // invalid port.
    }
    if (gd32_init_bootloader(port) < 0) {
        printf("can not init bootloader.\n", name);
        return;     // invalid protocol.
    }

    // init bootloader serial connection.
    id = gd32_get_unique_id(port);
    if (id == NULL) {
        printf("can not connect to chip.\n");
        return;
    }
    printf("connected to chip, id is %s.\n", id);

    // path is null, just read id but not read anything to file.
    if (path == NULL)
        return;

    // everything is ok, read data out, 64KB default.
    fp = fopen(path, "wb");
    if (fp == NULL) {
        printf("can not save to file %s.\n", path);
        goto read_end;
    }
    printf("[GD32] => %s: ", path);
    for (i = 0; i < 65536 / BLK_SIZE; i++) {       // 64KB totally
        char buf[BLK_SIZE] = {0};
        int size, used;

        size = gd32_read_memory(port, 0x08000000 + i * BLK_SIZE, buf, BLK_SIZE);
        if (size != BLK_SIZE) {
            printf("error: read size %d!=%d at block %d.\n", size, BLK_SIZE, i);
            break;
        }

        used = fwrite(buf, 1, size, fp);
        if (size != used) {
            printf("error: can not write data block %d.\n", i);
            break;
        }
        if (i % (2048 / BLK_SIZE) == 0) {
            fwrite("#", 1, 1, stdout);
            fflush(stdout);
        }
    }
    fclose(fp);
    printf("\n");       // end of transfer process line.

read_end:
    printf("elapsed time %lds, thank you.\n", time(NULL) - ct);

    gd32_uninit_serial(port);
}

void gd32_run_flash(struct sp_port *port)
{
    char buf[5];

    // jump command is 0x21.
    buf[0] = 0x21;
    buf[1] = ~buf[0];
    sp_write(port, buf, 2);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return;

    // flash default address is 0x08000000
    buf[0] = 0x08;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = block_xor(buf, 4);
    sp_write(port, buf, 5);
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return;

    // the bootloader will return another 0x79.
    if (1 == sp_read(port, buf, 1) && buf[0] != 0x79)
        return;

    // every thing is OK now.
    printf("run firmware from 0x08000000 now!\n");
}

void gd32_write_file_to_flash(const char *name, const char *path)
{
    struct sp_port *port;

    FILE *fp;
    int i;
    time_t ct = time(NULL);
    const char *id = NULL;

    port = gd32_init_serial(name);
    if (port == NULL) {
        printf("can not open serial %s.\n", name);
        return;     // invalid port.
    }
    if (gd32_init_bootloader(port) < 0) {
        printf("can not init serial %s.\n", name);
        return;     // invalid protocol.
    }

    // init bootloader serial connection.
    id = gd32_get_unique_id(port);
    if (id == NULL) {
        printf("can not connect to chip.\n");
        return;
    }
    printf("connected to chip, id is %s.\n", id);

    // erase all chip flash first.
    if (gd32_erase_flash(port) < 0) {
        printf("failed to erase chip.\n");
        return;
    }

    // everything is ok, write data to flash.
    fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("can not read file %s, erased only.\n", path);
        goto write_end;
    }
    printf("[GD32] <= %s: ", path);
    for (i = 0; ; i++) {
        char buf[BLK_SIZE];
        int size, used;

        size = fread(buf, 1, BLK_SIZE, fp);
        if (size <= 0) {
            // we have reached the end of the file ...
            // or might a rare error, ignore it. :)
            break;
        }

        used = gd32_write_memory(port, 0x08000000 + i * BLK_SIZE, buf, size);
        if (used != size) {
            printf("error: write size %d!=%d at block %d.\n", size, used, i);
            break;
        }
        
        if (i % (2048 / BLK_SIZE) == 0) {
            fwrite("#", 1, 1, stdout);
            fflush(stdout);
        }
    }
    fclose(fp);
    printf("\n");       // end of transfer process line.

    gd32_run_flash(port);

write_end:
    printf("elapsed time %lds, thank you.\n", time(NULL) - ct);

    gd32_uninit_serial(port);
}

int block_hex(const char *s, int size)
{
    int o = 0, i;
    for (i = 0; i < size; i++) {
        o = o << 4;
        if (s[i] >= '0' && s[i] <= '9')
            o += s[i] - '0';
        if (s[i] >= 'a' && s[i] <= 'f')
            o += s[i] - 'a' + 0xa;
        if (s[i] >= 'A' && s[i] <= 'F')
            o += s[i] - 'A' + 0xA;
    }
    return o;
}

int convert_hex_to_bin(const char *hex, const char *bin)
{
    FILE *fb, *fh;
    char buf[0x100], *p;
    int i, size, addr = 0, total = 0;

    fb = fopen(bin, "wb");
    fh = fopen(hex, "rb");
    if (fh == NULL || fb == NULL)
        return -1;

    while (1) {
        p = fgets(buf, 0x100, fh);
        if (p == 0)
            break;

        // skip the row of high address offset 04.
        // skip the row of last line 01.
        if (memcmp(buf + 7, "00", 2))
            continue;  // data type should be 00.
        if (addr != block_hex(buf + 3, 4))
            return -3;  // data address do not continuity.

        size = block_hex(buf + 1, 2);
        for (i = 0; i < size; i++) {
            char c = block_hex(buf + 9 + i * 2, 2) & 0xff;
            total += fwrite(&c, 1, 1, fb);
        }
        addr += size;
    }

    fclose(fb);
    fclose(fh);
    return total;
}

int convert_bin_to_hex(const char *bin, const char *hex)
{
    FILE *fb, *fh;
    char buf[0x20];
    int i, size, addr = 0, total = 0;

    fb = fopen(bin, "rb");
    fh = fopen(hex, "wb");
    if (fh == NULL || fb == NULL)
        return -__LINE__;

    // write 0x08000000 offset to hex file.
    total += fprintf(fh, ":020000040800F2\n");

    while (1) {
        size = fread(buf, 1, 0x20, fb);
        if (size <= 0)
            break;
        total += fprintf(fh, ":%02X%04X00", size, addr);
        for (i = 0; i < size; i++)
            fprintf(fh, "%02X", (unsigned char)buf[i]);
        fprintf(fh, "%02X\n", (unsigned char)block_xor(buf, 0x20));
        addr += size;
    }

    total += fprintf(fh, ":00000001FF");

    fclose(fb);
    fclose(fh);
    return total;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        printf("usage: gd32up list\n\tlist current valid serial ports.\n\n");
        printf("usage: gd32up read|write [port] [file bin]\n\tread/write bin file from/to flash.\n\n");
        printf("usage: gd32up hex2bin [in hex] [out: bin]\n\tconvert hex to bin file.\n\n");
        printf("usage: gd32up bin2hex [in bin] [out: hex]\n\tconvert bin to hex file.\n\n");
        return -1;
    }

    if (!strcmp(argv[1], "list")) {
        print_serial_list();
        return 1;
    }

    if (!strcmp(argv[1], "read")) {
        if (argc == 4)
            gd32_read_flash_to_file(argv[2], argv[3]);
        else
            gd32_read_flash_to_file(argv[2], NULL); 
        return 1;
    }

    if (!strcmp(argv[1], "write")) {
        // convert to hex if input is hex.
        int len = strlen(argv[3]);
        char *path = (char *)malloc(len + 1);
        strcpy(path, argv[3]);
        if(!strcmp(argv[3] + len - 4, ".hex")) {
            strcpy(path + len - 4, ".bin");
            convert_hex_to_bin(argv[3], path);
        }

        gd32_write_file_to_flash(argv[2], path);

        free(path);
        return 1;
    }

    if (!strcmp(argv[1], "hex2bin")) {
        printf("output file size: %d\n", convert_hex_to_bin(argv[2], argv[3]));
        return 1;
    }

    if (!strcmp(argv[1], "bin2hex")) {
        printf("output file size: %d\n", convert_bin_to_hex(argv[2], argv[3]));
        return 1;
    }
    return 0;
}
