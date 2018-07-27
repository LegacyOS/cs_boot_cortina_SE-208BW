#!/bin/sh

gcc -c sl_crc16.c 
gcc -c sl_imghdr.c 
gcc -o cs_imghdr sl_imghdr.o sl_crc16.o
