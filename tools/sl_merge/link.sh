#!/bin/sh

if gcc sl_merge.c -o cs_merge
then
	echo "done!";
else
	echo "error!";
fi
