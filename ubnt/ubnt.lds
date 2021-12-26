/* vim: ts=4:sw=4:expandtab */

    /* Copyright Ubiquiti Networks Inc. 2014
    ** All Rights Reserved.
    */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

/*
    NOTE: need to update UBNT_APP_START_OFFSET in ubnt_api_call.c if
          UBNT_PERSISTENT_DATA or UBNT_HEADER is changed.
*/
MEMORY {
    UBNT_HEADER (r) : ORIGIN = 0x4007ff28, LENGTH = 16
    UBNT_PERSISTENT_DATA (r) : ORIGIN = 0x4007ff48, LENGTH = 128
    UBNT_START (rx) : ORIGIN = 0x4007ffc8, LENGTH = 7k
}

ENTRY(ubnt_app)

SECTIONS
{
        ubnt_magic :
        {
            KEEP(*(.ubnt_hdr))
            common/libucommon.a (.ubnt_hdr)
        } > UBNT_HEADER

        ubnt_data :
        {
            KEEP(*(.ubnt_data))
        } > UBNT_PERSISTENT_DATA

        __ubnt_start = .;
        ubnt_start :
        {
            common/libucommon.a (.ubnt_start)
        } > UBNT_START

	. = LOADADDR(ubnt_start) + SIZEOF(ubnt_start) + 0x32;

	.text . : { *(.text) *(.text.*) }

	.rodata . : { *(.rodata) *(.rodata.*) }

	.data . : { *(.data) *(.data.*) }

	.sdata . : { *(.sdata) }

        .scommon . : { *(.scommon) }

        .data.rel . : { *(.data.rel) *(.data.rel.*) }

	_gp = ALIGN(16);

	__got_start = . ;
	.got . : { *(.got.plt) *(.got) }
	__got_end = .;

        __ubnt_app_end = . ;

        /* Move the BSS down so that the key material can be copied to RAM
        ** in a known location.
        */

	. = ALIGN(16);
	__bss_start = . + 0x200;
        . = __bss_start;
	.sbss . : { *(.sbss) }
	.bss  . : { *(.bss)  *(.bss.*) }
        _end = .;

}
