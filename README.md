# gd32tools

- upload firmwares to gd32f150 chips through serial port.
- compatible with stm32.
- accept hex and bin format.

----------------------------

### Firmware Toolchain

We use default ARM-M3 toolchain to compile, download at https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

### Note

- upload address is 0x08000000, the address in hex file is ignored.
- connect to gd32f150 uart1(pa9, pa10), boot0 should keep high.
- if your application can not work after load complete, try to add `NVIC_VectTableSet(NVIC_VECTTAB_FLASH, 0)` at start of main().

----------------------------

### Get libserialport

- git clone git://sigrok.org/libserialport
- ./autogen.sh
- ./configure
- make

the libserialport.a will be in its ./.libs/ folder.


### Compile serial debug for MacOS

- move libserialport to gd32up folder.

- gcc -g gd32up.c ./libserialport/.lib/libserialport.a -o gd32up -I./libserialport -framework IOKit -framework CoreFoundation


### Usage of gd32up

- list: list current valid serial ports.
- read|write [port] [file bin]: read/write bin file from/to flash.
- hex2bin [in hex] [out: bin]: convert hex to bin file.
- bin2hex [in bin] [out: hex]: convert bin to hex file.
- write [port]: erase flash only.


