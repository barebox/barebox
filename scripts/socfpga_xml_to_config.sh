#!/bin/bash

## TODO:
## - read in mpuclk and nocclk, must be calculated by hand at the moment
## - read in cfg_dedicated_io_*, must be calculated by hand at the moment

if [ "$#" -lt "2" ]
then
    echo "USAGE: $0 <boarddir> <HPS.xml>"
    exit 1
fi

dir=$1
xml=$2

pll_config() {
    local src
    local tgt
    src=$1
    tgt=$2

    MAINPLL=`grep mainpll "$src" | \
         sed -e 's/^.*mainpllgrp\.//g' | \
         sed -e 's/\./_/g' | \
         sed -e "s/' value/ /g" | \
         sed -e "s/'/ /g" | \
         sed -e "s#  />#,#g" | \
         sed -e "s/^/\t./g" |
         sort`

    # FIXME: Find solution
    MAINPLL_FIXME=".mpuclk = FIXME,
    .nocclk = FIXME,"

    PERPLL=`grep perpll "$src" | \
        sed -e 's/^.*perpllgrp\.//g' | \
        sed -e 's/\./_/g' | \
        sed -e "s/' value/ /g" | \
        sed -e "s/'/ /g" | \
        sed -e "s#  />#,#g" | \
        sed -e "s/^/\t./g" |
        sort`

    echo "#include <mach/arria10-clock-manager.h>" > $tgt
    echo >> $tgt
    echo "static struct arria10_mainpll_cfg mainpll_cfg = {" >> $tgt
    echo "$MAINPLL" >> $tgt
    echo "$MAINPLL_FIXME" >> $tgt
    echo "};" >> $tgt
    echo >> $tgt
    echo "static struct arria10_perpll_cfg perpll_cfg = {" >> $tgt
    echo "$PERPLL" >> $tgt
    echo "};" >> $tgt

    dos2unix $tgt
}

pinmux_config() {
    local src
    local tgt
    src=$1
    tgt=$2

    SHARED=`grep pinmux_shared "$src" | \
            sed -e 's/^.*pinmux_/[arria10_pinmux_/g' | \
            sed -e "s/\.sel' value='/] = /g" | \
            sed -e "s/' \/>/,/g"`

    DEDICATED=`grep pinmux_dedicated "$src" | \
            sed -e 's/^.*pinmux_/[arria10_pinmux_/g' | \
            sed -e "s/\.sel' value='/] = /g" | \
            sed -e "s/' \/>/,/g"`

    # FIXME: Either find solution how to parse these values too or replace
    # script with something that goes more in the direction of a programming
    # language
    DEDICATED_FIXME="[arria10_pincfg_dedicated_io_bank] = FIXME,
    [arria10_pincfg_dedicated_io_1] = FIXME,
    [arria10_pincfg_dedicated_io_2] = FIXME,
    [arria10_pincfg_dedicated_io_3] = FIXME,
    [arria10_pincfg_dedicated_io_4] = FIXME,
    [arria10_pincfg_dedicated_io_5] = FIXME,
    [arria10_pincfg_dedicated_io_6] = FIXME,
    [arria10_pincfg_dedicated_io_7] = FIXME,
    [arria10_pincfg_dedicated_io_8] = FIXME,
    [arria10_pincfg_dedicated_io_9] = FIXME,
    [arria10_pincfg_dedicated_io_10] = FIXME,
    [arria10_pincfg_dedicated_io_11] = FIXME,
    [arria10_pincfg_dedicated_io_12] = FIXME,
    [arria10_pincfg_dedicated_io_13] = FIXME,
    [arria10_pincfg_dedicated_io_14] = FIXME,
    [arria10_pincfg_dedicated_io_15] = FIXME,
    [arria10_pincfg_dedicated_io_16] = FIXME,
    [arria10_pincfg_dedicated_io_17] = FIXME"

    FPGA=`grep _fpga_interface_grp "$src" | \
          grep -v -e usb -e pll_clock_out | \
            sed -e 's/^.*pinmux_/[arria10_pinmux_/g' | \
            sed -e "s/\.sel' value='/] = /g" | \
            sed -e "s/' \/>/,/g"`

    echo "#include <mach/arria10-pinmux.h>" > $tgt
    echo >> $tgt
    echo "static uint32_t pinmux[] = {" >> $tgt
    echo "$SHARED" >> $tgt
    echo "$DEDICATED" >> $tgt
    echo "$DEDICATED_FIXME" >> $tgt
    echo "$FPGA" >> $tgt
    echo "};" >> $tgt
    echo >> $tgt

    dos2unix $tgt
}

pll_config $xml $dir/pll-config-arria10.c

pinmux_config $xml $dir/pinmux-config-arria10.c
