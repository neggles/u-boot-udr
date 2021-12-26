#!/bin/bash

# CONF expects to match the suffix of configure file name
# PRODUCT_TYPE expects the first token of '$CONF'
#
# Example: the config file 'config-uap-nano-hd'
# CONF=uap-nano-hd
# PRODUCT_TYPE=uap
CONF=udr
PRODUCT_TYPE=udr
UABOARD=mt7622
UAVENDOR=mtk
CROSS_PATH=${CROSS_PATH:-/opt/buildroot-gcc492_arm/usr/bin}
CROSS_COMPILE=arm-linux-
UBOOT_DEBUG="V=s verbose=1"
MK_EXTRA="$UBOOT_DEBUG CROSS_COMPILE=$CROSS_COMPILE"
BRANCH=

JOBS=${JOBS:=$(nproc)}
BUILD_DEBUG=${BUILD_DEBUG:=1}
BUILD_RELEASE=${BUILD_RELEASE:=1}

PROGRAM=$(basename $0)
PRODUCTS=$(for p in ./config-*; do [ -e $p ] || continue; printf "${p##*config-} "; done)

ubnt_verify_app()
{
    UAPP_START=`${CROSS_COMPILE}objdump -hww ./ubnt/ubnt_app | grep "ubnt_magic" | sed -e 's@^ *@@' | tr -s " " | cut -d ' ' -f 4`
    UAPP_START=0x$UAPP_START
    UAPP_END=`${CROSS_COMPILE}objdump -hww ./ubnt/ubnt_app | grep "bss" | sed -e 's@^ *@@' | tr -s " " | cut -d ' ' -f 4`
    UAPP_END=0x$UAPP_END
    UAPP_SIZE=$(($UAPP_END - $UAPP_START))

    UBOOT_PART_SIZE=0x50000
    UBOOT_SIZE=`stat -c %s u-boot.bin`
    UAPP_LOAD_SIZE=$(($UBOOT_PART_SIZE - $UBOOT_SIZE))

    if [ $UAPP_SIZE -gt $UAPP_LOAD_SIZE ]; then
        echo "ubnt_app size is bigger than load size!"
        echo "UAPP_START: $UAPP_START UAPP_END: $UAPP_END UAPP_SIZE: $UAPP_SIZE, UAPP_LOAD_SIZE: $UAPP_LOAD_SIZE"
        return 1
    fi

    UAPP_RUN_IMAGE_START=`cat ./common/ubnt_api_calls.c | grep "define UBNT_APP_IMAGE_OFFSET" | sed -e 's@^ *@@' | tr -s " "| cut -d ' ' -f 3-`
    UAPP_START_DIFF=$(($UAPP_RUN_IMAGE_START - $UAPP_START))
    if [ $UAPP_START_DIFF -ne 0 ]; then
        echo "ubnt_app load address ($UAPP_RUN_IMAGE_START) is not equal to the address in file ($UAPP_START)"
        return 1
    fi

    UBNT_DATA_START=`${CROSS_COMPILE}objdump -hww ./ubnt/ubnt_app | grep "ubnt_data" | sed -e 's@^ *@@' | tr -s " " | cut -d ' ' -f 4`
    UBNT_DATA_START=0x$UBNT_DATA_START
    UAPP_RUN_DATA_ADDR=`cat ./common/ubnt_api_calls.c | grep "define UBNT_APP_SHARE_DATA_OFFSET" | sed -e 's@^ *@@' | tr -s " "| cut -d ' ' -f 3-`
    UAPP_DATA_DIFF=$(($UBNT_DATA_START - $UAPP_RUN_DATA_ADDR))
    if [ $UAPP_DATA_DIFF -ne 0 ]; then
        echo "ubnt_app persistent data address ($UAPP_RUN_DATA_ADDR) is not equal to the address in file ($UBNT_DATA_START)"
        return 1
    fi

    UBNT_START=`${CROSS_COMPILE}objdump -hww ./ubnt/ubnt_app | grep "ubnt_start" | sed -e 's@^ *@@' | tr -s " " | cut -d ' ' -f 4`
    UBNT_START=0x$UBNT_START
    UAPP_RUN_ADDR=`cat ./common/ubnt_api_calls.c | grep "define UBNT_APP_START_OFFSET" | sed -e 's@^ *@@' | tr -s " "| cut -d ' ' -f 3-`
    UAPP_RUN_DIFF=$(($UBNT_START - $UAPP_RUN_ADDR))
    if [ $UAPP_RUN_DIFF -ne 0 ]; then
        echo "ubnt_app start address ($UAPP_RUN_ADDR) is not equal to the address in file ($UBNT_START)"
        return 1
    fi

    echo "============================================================"
    echo "          All ubnt_app loading points verified!"
    echo "============================================================"
    return 0
}

# Get flash size(MiB) from the partition layout parameters
# in the configuration file.
#
# args: <config file>
# returns: string of flash size in MB. Ex. '32'
get_flash_sz_from_config()
{
    local size
    [ -e $1 ] || echo ""

    . $1
    size=$(( ( 384 + \
               $UBNT_PART_UBOOT_KSIZE + \
               $UBNT_PART_UBOOT_ENV_KSIZE + \
               $UBNT_PART_FACTORY_KSIZE + \
               $UBNT_PART_EEPROM_KSIZE + \
               $UBNT_PART_BOOTSELECT_KSIZE + \
               $UBNT_PART_CFG_KSIZE + \
               $UBNT_PART_KERNEL0_KSIZE + \
               $UBNT_PART_KERNEL1_KSIZE ) / 1024 ))

    if [ -z "$size" ] || [ $size -eq 0 ]; then
        echo ""
    else
        echo "${size}"
    fi
}

prepare_artifacts()
{
    local BUILD_TYPE=$1
    local BIN=$2

    if [ -f ${BIN} ] ; then
        local fn_infix flash_sz
        VER=$(./gen_unifi_version.sh | sed 's,unifi-ubnt/,,g' | sed 's,/,-,g')
        fn_infix="${PRODUCT_TYPE}-${UABOARD}"

        if $BUILD_UBNT_APP; then
            flash_sz=$(get_flash_sz_from_config ".config")
            [ -n "${flash_sz}" ] && fn_infix="${fn_infix}-${flash_sz}MB"
        fi

        if [ $BUILD_TYPE = debug ] ; then
            cp ${BIN} ${VER}_${fn_infix}_debug_u-boot.bin
        else
            cp ${BIN} ${VER}_${fn_infix}_nodebug_u-boot.bin
            ln -s  ${VER}_${fn_infix}_nodebug_u-boot.bin ${VER}_${fn_infix}_u-boot.bin
        fi
        echo "${VER}" > .version
    else
        echo "Binary ${BIN} not found!"
        exit 1
    fi
}

ubnt_build()
{
    make ${MK_EXTRA} -C ubnt $1 $2 VENDOR=${UAVENDOR} BOARD=${UABOARD} clean
    make -j$JOBS ${MK_EXTRA} -C ubnt $1 $2 VENDOR=${UAVENDOR} BOARD=${UABOARD} all
    ret=$?
    if [ ${ret} -ne 0 ] ; then
        echo "make failed with return code ${ret}!"
        exit ${ret}
    fi

    if [ $1 = "DEBUG=1" ] ; then
        prepare_artifacts debug u-boot-with-app-debug.bin
    else
        prepare_artifacts release u-boot-with-app.bin
    fi
}

usage()
{
    printf "$PROGRAM [OPTIONS] {$(echo $PRODUCTS | tr ' ' '|')} [ubnt make targets]\n"
    if [ x$1 = x-h ]; then
        printf "
    OPTIONS:
        -h    Print this usage and exit
        -b    Reference by branch. Reference by tag if not set.
"
        exit 0
    else
        exit 1
    fi
}

#### MAIN ####

options=`getopt -o b:h -- "$@"`

if [ $? -ne 0 ]
then
    usage
fi

eval set -- "$options"

while true
do
    case "$1" in
        -h)
            usage -h ;;
        -b)
            BRANCH="$2"
            shift 2 ;;
        --)
            shift
            CONF="$1"
            MK_TARGET="$2"
            PRODUCT_TYPE="${CONF%%-*}"
            break ;;
    esac
done

if [ -z "$CONF" ]
then
    usage
fi

[ -e "config-$CONF" ] || {
    echo "[ERROR] Configuration file [config-$CONF] doesn't exist!"
    usage
}

case "$CONF" in
    uap6-pro) BUILD_UBNT_APP=true;;
    *) BUILD_UBNT_APP=false;;
esac

function print_toolchain_instructions ()
{
    echo "   Set CROSS_PATH to a valid location"
    echo " -or-"
    echo "   Install the toolchain with this command:"
    echo "     curl https://slc-nas-egadd.rad.ubnt.com/dl/buildroot-gcc492_arm.tar.bz2 |sudo tar -xjC /opt"
    echo
}

if [ ! -d "${CROSS_PATH}" ]
then
    echo
    echo "[ERROR] CROSS_PATH ${CROSS_PATH} doesn't exist!!"
    print_toolchain_instructions
    exit 1
fi

if [ ! -x "${CROSS_PATH}/${CROSS_COMPILE}gcc" ]
then
    echo
    echo "[ERROR] no executable ${CROSS_COMPILE}gcc found!!"
    print_toolchain_instructions
    exit 1
fi

if [ "${BRANCH}" != "" ]; then
    export BRANCH=${BRANCH}
fi

export PATH=${PATH}:${CROSS_PATH}

#SHARE_KEY_DIR=./ubnt/unifi-key
#SHARE_KEY_GIT_URL=http://10.1.0.9/unifi/git/unifi-key.git
#if [ ! -d ${SHARE_KEY_DIR} ]; then
# git clone ${SHARE_KEY_GIT_URL} ${SHARE_KEY_DIR};
#fi

$BUILD_UBNT_APP && make ${MK_EXTRA} -C ubnt $MK_TARGET VENDOR=${UAVENDOR} BOARD=${UABOARD} clean
make ${MK_EXTRA} mrproper
ret=$?
if [ ${ret} -ne 0 ] ; then
    echo "make mrproper failed with return code ${ret}!"
    exit ${ret}
fi
rm -vf unifi-*u-boot.bin
rm -vf *.tmp

. gen_unifi_version.sh
export VERSION_CODE=UniFi
export REVISION=${UNIFI_REVISION}

# Copy the config
cp config-${CONF} .config

# Use menuconfig save functions to generate the autoconf.h
_message=$(make menuconfig MENUCONFIG_SAVE_IMMEDIATELY=1)
ret=$?

case $_message in
    *"Your display is too small"*) _err=1;;
    *) _err=0;;
esac

if [ ${ret} -ne 0 ] || [ $_err -ne 0 ] ; then
    echo "make menuconfig failed with return code ${ret}!"
    echo "$_message"
    exit ${ret}
fi

if [ "$BUILD_DEBUG" -eq "1" ] ; then
    make -j$JOBS ${MK_EXTRA} clean DEBUG=1 || { echo "u-boot build failed!"; exit -1; }
    make -j$JOBS ${MK_EXTRA} DEBUG=1 || { echo "u-boot build failed!"; exit -1; }
    if $BUILD_UBNT_APP; then
        ubnt_build DEBUG=1 || { echo "debug build failed!";  exit -1; }
        ubnt_verify_app
        ret=$?
        if [ ${ret} -ne 0 ] ; then
            echo "ubnt_verify_app failed with return code ${ret}!"
            exit ${ret}
        fi
    else
        prepare_artifacts debug u-boot-mtk.bin
    fi
fi

if [ "$BUILD_RELEASE" -eq "1" ] ; then
    make -j$JOBS ${MK_EXTRA} clean DEBUG=0 || { echo "u-boot build failed!"; exit -1; }
    make -j$JOBS ${MK_EXTRA} DEBUG=0 || { echo "u-boot build failed!"; exit -1; }
    if $BUILD_UBNT_APP; then
        ubnt_build DEBUG=0 || { echo "release build failed!";  exit -2; }
        ubnt_verify_app
        ret=$?
        if [ ${ret} -ne 0 ] ; then
            echo "ubnt_verify_app failed with return code ${ret}!"
            exit ${ret}
        fi
    else
        prepare_artifacts release u-boot-mtk.bin
    fi
fi

#-------------------------------------------------------------------------------
#
# Local Variables:
# mode: shell-script
# indent-tabs-mode: nil
# sh-indentation: 4
# sh-basic-offset: 4
# tab-width: 4
# End:
