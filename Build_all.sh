#!/bin/sh
CurDir=`pwd`
mkdir -p /release
chmod 777 -R /release
rm -f /release/*
cd /source/cs-boot/board/nbuild_all/build/
echo "Start build P-flash 16M single cpu..." 
chmod 777 /source/cs-boot/board/nbuild_all/build/
rm -f makefile
rm -f ../inc/board_config.h
rm -f ../inc/board_version.h
rm -f ../bin/*

ln -s makefile_1cpu16m makefile
ln -s ../inc/board_config.h_1cpu16m ../inc/board_config.h 
ln -s ../inc/board_version.h_1cpu16m ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AN_P16M.bin /release/lepus_linux_SL0AP16.bin

rm -f makefile
rm -f ../inc/board_config.h
rm -f ../inc/board_version.h

echo "Start build P-flash 16M N single cpu..." 
chmod 777 /source/cs-boot/board/nbuild_all/build/
rm -f makefile
rm -f ../inc/board_config.h
rm -f ../inc/board_version.h
rm -f ../bin/*

ln -s makefile_1cpu16mn makefile
ln -s ../inc/board_config.h_1cpu16mn ../inc/board_config.h 
ln -s ../inc/board_version.h_1cpu16mn ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AN_P16MN.bin /release/lepus_linux_SL0AP16N.bin

rm -f makefile
rm -f ../inc/board_config.h
rm -f ../inc/board_version.h

echo "Start build N-flash 64M single cpu..." 
ln -s makefile_1cpu64m makefile
ln -s ../inc/board_config.h_1cpu64m ../inc/board_config.h 
ln -s ../inc/board_version.h_1cpu64m ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AN_N64M.bin /release/lepus_linux_SL0AN64.bin

rm -f makefile
rm -f ../inc/board_config.h
rm -f ../inc/board_version.h

echo "Start build N-flash 1G single cpu..." 
ln -s makefile_1cpu1g makefile
ln -s ../inc/board_config.h_1cpu1g ../inc/board_config.h 
ln -s ../inc/board_version.h_1cpu1g ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AN_N1G.bin /release/lepus_linux_SL0AN_N1G.bin

rm -f makefile
rm -f ../inc/board_config.h 
rm -f ../inc/board_version.h 

echo "Start build P-flash 8M single cpu..." 
ln -s makefile_1cpu8m makefile
ln -s ../inc/board_config.h_1cpu8m ../inc/board_config.h 
ln -s ../inc/board_version.h_1cpu8m ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AN_P8M.bin /release/lepus_linux_SL0AP.bin

rm -f makefile
rm -f ../inc/board_config.h 
rm -f ../inc/board_version.h 

echo "Start build HD Boot single cpu..." 
ln -s makefile_1cpuhd makefile
ln -s ../inc/board_config.h_1cpuhd ../inc/board_config.h 
ln -s ../inc/board_version.h_1cpuhd ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AP_HDBOOT.bin /release/lepus_linux_SL0AP_HD.bin

rm -f makefile
rm -f ../inc/board_config.h 
rm -f ../inc/board_version.h 

echo "Start build P-flash 16M dual cpu..." 
ln -s makefile_2cpu16m makefile
ln -s ../inc/board_config.h_2cpu16m ../inc/board_config.h 
ln -s ../inc/board_version.h_2cpu16m ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AN_P16M_2CPU.bin /release/lepus_linux_SL0AP16_2CPU.bin

rm -f makefile
rm -f ../inc/board_config.h 
rm -f ../inc/board_version.h 

echo "Start build N-flash 64M dual cpu..." 
ln -s makefile_2cpu64m makefile
ln -s ../inc/board_config.h_2cpu64m ../inc/board_config.h 
ln -s ../inc/board_version.h_2cpu64m ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AN_N64M_2CPU.bin /release/lepus_linux_SL0AN64_2CPU.bin

rm -f makefile
rm -f ../inc/board_config.h 
rm -f ../inc/board_version.h 

echo "Start build HD Boot dual cpu..." 
ln -s makefile_2cpuhd makefile
ln -s ../inc/board_config.h_2cpuhd ../inc/board_config.h 
ln -s ../inc/board_version.h_2cpuhd ../inc/board_version.h 

make clean all;

cp -f ../bin/lepus_linux_SL0AP_HDBOOT_2CPU.bin /release/lepus_linux_SL0AP_HD_2cpu.bin

rm -f makefile
rm -f ../inc/board_config.h 
rm -f ../inc/board_version.h 

cd ${CurDir}
echo "Build boot end..." 

