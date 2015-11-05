#pragma once
#include <stdint.h>
/*
OS内存布局:
物理地址                           虚拟地址
00000000 +-----------------------+ 0x80000000
         |    boot_code(<60k)    |
		 |       ...             |
		 |    boot stack         |
00010000 +-----------------------+ 0x80010000
		 |    video_buffer       |
00020000 +-----------------------+ 0x80020000
         |    system_info        |
		 |  (guestOS PE header   |
00021000 +-----------------------+ 0x80021000
		 |    guestOS body       |
		 |       ...             |
         +-----------------------+
		 |    mem_block_list     |
		 |       ...             |
		 |    block_minidump     |
		 +-----------------------+
		 |    shellcode buffer   |
		 |       ...             |
		 +-----------------------+
		 |   free_page_frame     |
		 |                       |
		 |                       |
		 |                       |
		 +-----------------------+
mem_block_list[0] = mem_block_os
mem_block_list[1] = mem_block_shellcode
mem_block_list[2] = mem_block_minidump
...
mem_block_list[n] = mem_block_minidump
*/

#define		VM_RAM_SIZE		(1024*1024*100)

#define		BOOT_CODE_BASE		0x00000000
#define		BOOT_CODE_STACK		0x00010000
#define     VIDEO_BUF_BASE		0x00010000
#define     VIDEO_BUF_SIZE		0x00010000
#define     OS_CODE_BASE		0x00020000
#define     VM_CONTEXT_BASE		OS_CODE_BASE


#define     SHELLCODE_BUF_SIZE  (1024*1024*10)

#define     BLOCK_GUESTOS		0
#define     BLOCK_SHELLCODE		1
#define     BLOCK_MINIDUMP		2

#define     PAGE_SIZE			 4096
#define     SIZE_TO_PAGES(size)	 ((uint32_t)(size + 4095)>>12)
#define     PAGES_TO_SIZE(pages) ((uint32_t)(pages)<<12)
#define     PAGE_ALGIN_SIZE(size)((uint32_t)(size + 4095)&0xFFFFF000)

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


