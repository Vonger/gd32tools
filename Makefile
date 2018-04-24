all:
	gcc -g gd32up.c ./libserialport.a -o gd32up -I./libserialport -framework IOKit -framework CoreFoundation
