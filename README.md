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

### Use GCC compile gd32f150 app

- download GD32F1x0_Firmware_Library_v3.1.0 to GD32GCC folder and uncompress.
- put toolchain to GD32GCC/toolchain folder, for example, if you are macos, put the toolchain to GD32GCC/toolchain/mac, if windows, put it to GD32GCC/toolchain/win. (Linux guy normally are super, guild is not necessary :)
- move the project folder in this gd32tools to GD32GCC folder.
- call make in GD32GCC/project/led. note: Makefile default path is for mac, if you want to run in Windows or Linux, must change the $(TOOLCHAIN) path.
- if everything works normal, you will get led.hex
- you can call `gd32up write /dev/ttyS0 led.hex`, write it to the chip. 


