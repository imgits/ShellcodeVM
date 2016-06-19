#pragma once
#include <stdint.h>
#include "paging.h"
/*
OS内存布局:
物理地址                           虚拟地址
00000000 +-----------------------+ 0x00000000
		 |    boot stack(64k)    |
00010000 +-----------------------+ 0x00010000
		 |  boot_code(1M-64K)    |
00100000 +-----------------------+ 0xC0400000
   	     |    frame_db91M)		 |
00200000 +-----------------------+ 0xC0500000
		 |    video_buffer(2M)   |
00400000 +------------+----------+ 0xC0000000
         | page_table |          |
00700000 +   (4M)     +----------+ 0xC0300000
		 |            |page_dir  |
00701000 +            +----------+ 0xC0301000
		 |            |          |
00800000 +-----------------------+ 0x80000000
		 |  (guestOS PE header   |
01000000 +-----------------------+ 0x88000000
		 |  shellcode            |
         +-----------------------+
		 |                       |
		 +-----------------------+

mem_block_list[0] = mem_block_os
mem_block_list[1] = mem_block_shellcode
mem_block_list[2] = mem_block_minidump
...
mem_block_list[n] = mem_block_minidump
*/

#define		VM_RAM_SIZE					MB(100)

#define		BOOT_CODE_STACK				0x00010000
#define		BOOT_CODE_PHYSICAL_ADDR		0x00010000
#define		BOOT_CODE_VIRTUAL_ADDR		0x00010000
#define		BOOT_CODE_SIZE				MB(1)

#define     FRAME_DB_PHYSICAL_ADDR		0x00100000
#define     FRAME_DB_VIRTUAL_ADDR		0xC0400000
#define     FRAME_DB_SIZE				MB(1)

#define     VIDEO_BUF_PHYSICAL_ADDR		0x00200000
#define     VIDEO_BUF_VIRTUAL_ADDR		0xC0500000
#define     VIDEO_BUF_SIZE				MB(2)

#define     PAGE_TABLE_PHYSICAL_ADDR	0x00400000
#define     PAGE_TABLE_VIRTUAL_ADDR		0xC0000000
#define     PAGE_TABLE_SIZE				MB(4)

#define     PAGE_DIR_PHYSICAL_ADDR		0x00700000 //PAGE_TABLE_PHYSICAL_ADDR + PED_INDEX(PAGE_TABLE_VIRTUAL_ADDR)*PAGE_SIZE
#define     PAGE_DIR_VIRTUAL_ADDR		0xC0300000 //PAGE_TABLE_VIRTUAL_ADDR + PED_INDEX(PAGE_TABLE_VIRTUAL_ADDR)*PAGE_SIZE

#define     OS_CODE_PHYSICAL_ADDR		0x00800000
#define     OS_CODE_VIRTUAL_ADDR		0x80000000
#define     OS_CODE_SIZE				MB(8)

#define     VM_CONTEXT_PHYSICAL_ADDR	OS_CODE_PHYSICAL_ADDR
#define     VM_CONTEXT_VIRTUAL_ADDR		OS_CODE_VIRTUAL_ADDR
#define     VM_CONTEXT_SIZE				PAGE_SIZE

#define     KERNEL_MEM_SIZE             MB(16)

#define     KERNEL_STACK_BOTTOM_PHYSICAL_ADDR        (0x00000000+PAGE_SIZE)
#define     KERNEL_STACK_TOP_PHYSICAL_ADDR            (FRAME_DB_PHYSICAL_ADDR-PAGE_SIZE)
#define     KERNEL_STACK_BOTTOM_VIRTUAL_ADDR          (0xC0700000+PAGE_SIZE)
#define     KERNEL_STACK_TOP_VIRTUAL_ADDR             (0xC0800000-PAGE_SIZE)
#define     KERNEL_STACK_SIZE						  (FRAME_DB_PHYSICAL_ADDR-2*PAGE_SIZE)

//#define     KERNEL_STACK_BOTTOM_PHYSICAL_ADDR        (0x00000000)
//#define     KERNEL_STACK_TOP_PHYSICAL_ADDR            (FRAME_DB_PHYSICAL_ADDR)
//#define     KERNEL_STACK_BOTTOM_VIRTUAL_ADDR          (0xC0700000)
//#define     KERNEL_STACK_TOP_VIRTUAL_ADDR             (0xC0800000)
//#define     KERNEL_STACK_SIZE						  (FRAME_DB_PHYSICAL_ADDR)


/*
SHELLCODE_STACK 0x00000000-0x000FFFFF		0x00000000-0x000FFFFF
SHELLCODE_BUF   0x00100000-0x00FFFFFF		0x00100000-0x00FFFFFF

OS_CODE			0x01000000-0x017FFFFF		0x80000000-0x807FFFFF
OS_STACK		0x01800000-0x018FFFFF       0x80800000-0x808FFFFF 
FRAME_DB		0x01900000-0x019FFFFF       0x80900000-0x809FFFFF
VIDEO_BUF		0x01A00000-0x01BFFFFF		0x80A00000-0x80BFFFFF
PAGE_TABLE		0x01C00000-0x01FFFFFF       0xC0000000-0xC03FFFFF
*/
//#define     SHELLCODE_BUF_SIZE  (1024*1024*10)

#define     BLOCK_GUESTOS		0
#define     BLOCK_SHELLCODE		1
#define     BLOCK_MINIDUMP		2

//#define     PAGE_SIZE			 4096
//#define     SIZE_TO_PAGES(size)	 ((uint32_t)(size + 4095)>>12)
//#define     PAGES_TO_SIZE(pages) ((uint32_t)(pages)<<12)
//#define     PAGE_ALGIN_SIZE(size)((uint32_t)(size + 4095)&0xFFFFF000)

#define DBG(msg) video.puts(msg); __asm mov eax, 0x111111111 __asm mov ebx, 0x2222222 __asm hlt

struct VM_CONTEXT
{
	uint32_t ram_base;
	uint32_t ram_size;
	uint32_t ram_used;
	uint32_t boot_start;

	uint32_t video_buf_base;
	uint32_t video_buf_size;

	uint32_t os_physcial_address;
	uint32_t os_virtual_address;
	uint32_t os_image_size;
	uint32_t os_entry_point;

	uint32_t shellcode_base;
	uint32_t shellcode_file_size;
	uint32_t shellcode_buf_size;

	//uint32_t kernel_code, kernel_data, user_code, user_data;
	//uint32_t fs, gs;
	uint32_t mem_blocks;
};

//物理页2*256
struct MEM_BLOCK_INFO
{
	uint32_t physical_address;
	uint32_t virtual_address;
	uint32_t size;
	uint32_t flags;
};


