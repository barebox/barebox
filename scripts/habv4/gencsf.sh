#!/bin/sh

set -e

while getopts "f:c:i:o:" opt; do
    case $opt in
	f)
	    file=$OPTARG
	    ;;
	c)
	    cfg=$OPTARG
	    ;;
	i)
	    in=$OPTARG
	    ;;
	o)
	    out=$OPTARG
	    ;;
	\?)
	    echo "Invalid option: -$OPTARG" >&2
	    exit 1
	;;
    esac
done

if [ ! -e $file -o ! -e $cfg -o ! -e $in ]; then
    echo "file not found!"
    exit 1
fi

#
# extract and set as shell vars:
# loadaddr=
# dcdofs=
#
eval $(sed -n -e "s/^[[:space:]]*\(loadaddr\|dcdofs\)[[:space:]]*\(0x[0-9]*\)/\1=\2/p" $cfg)

length=$(stat -c '%s' $file)

sed -e "s:@TABLE_BIN@:$TABLE_BIN:" \
    -e "s:@CSF_CRT_PEM@:$CSF_CRT_PEM:" \
    -e "s:@IMG_CRT_PEM@:$IMG_CRT_PEM:" \
    -e "s:@LOADADDR@:$loadaddr:" \
    -e "s:@OFFSET@:0:" \
    -e "s:@LENGTH@:$length:" \
    -e "s:@FILE@:$file:" \
    $in > $out
