/*****************************************************************************
 File:multiboot.h
 Description:Definition of Multiboot header for C and Assembler

*****************************************************************************/
#ifndef __MULTIBOOT__H
#define __MULTIBOOT__H

/* Physical address where kernel is loaded    */
#define DEF_MM_KERNEL_PHY_BASEADDR    0x00100000

/*
==================================================================================

    DEFINES of Multiboot header for C and Assember

==================================================================================
*/
/*
----------------------------------------------------------------------------------

    Magic of Multiboot Header

----------------------------------------------------------------------------------
*/
#define DEF_MBH_MAGIC            0x1BADB002     /* magic                        */

/*
----------------------------------------------------------------------------------

    Flags of Multiboot Header

----------------------------------------------------------------------------------
*/
#define DEF_MBH_FLAGS_PAGE_ALIGN    0x00000001  /* page (4KB) boundaries        */
#define DEF_MBH_FLAGS_MEMORY_INFO   0x00000002  /* memory information           */
#define DEF_MBH_FLAGS_VIDEO_MODE    0x00000004  /* video mode                   */
#define DEF_MBH_FLAGS_AOUT_KLUDGE   0x00010000  /* use address fields           */

/*
----------------------------------------------------------------------------------

    Other fileds of Multiboot Header

----------------------------------------------------------------------------------
*/
/* Flags for elf kernel image                          */
#define DEF_MBH_FLAGS           ( DEF_MBH_FLAGS_PAGE_ALIGN | DEF_MBH_FLAGS_MEMORY_INFO | DEF_MBH_FLAGS_VIDEO_MODE )
/* Flags for binary kernel image                       */
#define DEF_MBH_FLAGS_BIN       ( DEF_MBH_FLAGS_PAGE_ALIGN | DEF_MBH_FLAGS_MEMORY_INFO | DEF_MBH_FLAGS_AOUT_KLUDGE )
#define DEF_MBH_CHECKSUM        ( - ( DEF_MBH_MAGIC + DEF_MBH_FLAGS     ) )
#define DEF_MBH_CHECKSUM_BIN    ( - ( DEF_MBH_MAGIC + DEF_MBH_FLAGS_BIN ) )
/* The following fields are unnecessory for elf format */
#define DEF_MBH_HEADER_ADDR     DEF_MM_KERNEL_PHY_BASEADDR
#define DEF_MBH_MODE_TYPE       0x00000001        /* text mode                  */
#define DEF_MBH_WIDTH           0x00000050        /* width  = 80                */
#define DEF_MBH_HEIGHT          0x00000019        /* height = 40                */
#define DEF_MBH_DEPTH           0x00000000

#endif  /* __MULTIBOOT__H */


/*
=================================================================================

    The followings are for only C

=================================================================================
*/
#ifndef __MULTIBOOT__S
#ifndef __MULTIBOOT_HEADER_H
#define __MULTIBOOT_HEADER_H

typedef unsigned int    UINT32;

/*
=================================================================================

    Multiboot Header

=================================================================================
*/
typedef struct
{
    UINT32  magic;
    UINT32  flags;
    UINT32  checksum;
    /* The followings are only valid if MULTIBOOT_AOUT_KLUDGE is set	*/
    UINT32  header_addr;
    UINT32  load_addr;
    UINT32  load_end_addr;
    UINT32  bss_end_addr;
    UINT32  entry_addr;
    /* The followings are only valid if MULTIBOOT_VIDEO_MODE is set		*/
    UINT32  mode_type;
    UINT32  width;
    UINT32  height;
    UINT32  depth;
}MULTIBOOT_HEADER;

/*
=================================================================================

    Multiboot Info

=================================================================================
*/
typedef struct
{
    UINT32  flags;          /* multiboot info version number     */
    /* available memory from BIOS       */
    UINT32  mem_lower;
    UINT32  mem_upper;
    UINT32  boot_device;    /* "root" partition                  */
    UINT32  cmdline;        /* kernel command line               */
    /* Boot-Module list                 */
    UINT32  mods_count;
    UINT32  mods_addr;

    UINT32  syms1;
    UINT32  syms2;
    UINT32  syms3;

    /* memory mapping buffer            */
    UINT32  mmap_length;
    UINT32  mmap_addr;
    /* drive info buffer                */
    UINT32  drives_length;
    UINT32  drives_addr;
    /* ROM configuration table          */
    UINT32  config_table;
    /* bootloader name                  */
    UINT32  boot_loader_name;
    /* Video                            */
    UINT32  vbe_control_info;
    UINT32  vbe_mode_info;
    UINT32  vbe_mode;
    UINT32  vbe_interface_seg;
    UINT32  vbe_interface_off;
    UINT32  vbe_interface_len;
    
    UINT32  framebuffer_addr[2];
    UINT32  framebuffer_pitch;
    UINT32  framebuffer_width;
    UINT32  framebuffer_height;
    UINT32  framebuffer_bpp;
    UINT32  framebuffer_type;
}MULTIBOOT_INFO;

#endif	/* __MULTIBOOT_HEADER_H  */
#endif	/* __MULTIBOOT__S        */
