# gd32tools

- upload firmwares to gd32f1xx chips.
- compatible with stm32.
- accept hex and bin format.

----------------------------

upload address is 0x08000000, the address in hex file is ignored.

----------------------------

### Get libserialport

- git clone git://sigrok.org/libserialport
- ./autogen.sh
- ./configure
- make

the libserialport.a will be in its ./.libs/ folder.


### Compile serial debug for MacOS

- gcc -g gd32up.c ./libserialport/.lib/libserialport.a -o gd32up -I./libserialport -framework IOKit -framework CoreFoundation


### Usage of gd32up

-list: list current valid serial ports.
-read|write [port] [file bin]: read/write bin file from/to flash.
-hex2bin [in hex] [out: bin]: convert hex to bin file.
-bin2hex [in bin] [out: hex]: convert bin to hex file.
-write [port]: erase flash only.


