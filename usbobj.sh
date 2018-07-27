#!/bin/sh                                                                       
echo "rm usb src..."
cd /source/sl-boot
rm -rf ./src/usb

echo " modify build.mak.... "
cat ./env/build.mak | sed s/"\$(MAKE) -r -C \$(SRCPATH)\/usb"/"#\$(MAKE) -r -C \$(SRCPATH)\/usb"/g > ./env/build.mak

echo "Flash boot copy libusb.a... " 
cp ./board/lepus_linux/lib/libusb.a ./board/lepus_linux/obj/usb/libusb.a
echo " modify boot2.ld.... "
cat ./board/lepus_linux/build/boot2.ld | sed s/".\/..\/lib\/libusb.a"/".\/..\/obj\/usb\/libusb.a"/g > ./board/lepus_linux/build/boot2.ld

#echo "HD boot copy libusb.a... " 
#cp ./board/lepus_linux_ide/lib/libusb.a ./board/lepus_linux_ide/obj/usb/libusb.a
#echo " modify boot2.ld.... "
#cat ./board/lepus_linux_ide/build/boot2.ld | sed s/".\/..\/lib\/libusb.a"/".\/..\/obj\/usb\/libusb.a"/g > ./board/lepus_linux_ide/build/boot2.ld
echo " end ...."
