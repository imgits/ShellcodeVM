#pragma once
#include <stdint.h>
/*
OS内存布局:
物理地址                           虚拟地址
00000000 +-----------------------+
         |    boot_code(<60k)    |
		 |       ...             |
		 |    boot stack         |
00010000 +-----------------------+
		 |    video_buffer       |
00020000 +-----------------------+
         |    system_info        |
00021000 +-----------------------+
		 |    block_guestos      |
		 |       ...             |
		 |    mem_block_list     |
		 |       ...             |
		 |    block_minidump     |
		 |       ...             |
		 |    block_shellcode    |
		 |       ...             |
???????? +-----------------------+
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

#define		BOOT_CODE_BASE		0x00000000
#define		BOOT_CODE_STACK		0x00010000
#define     VIDEO_BUF_BASE		0x00010000
#define     VIDEO_BUF_SIZE		0x00010000
#define     OS_INFO_BASE		0x00020000
#define     OS_CODE_BASE		0x00021000

#define     SHELLCODE_BUF_SIZE  (1024*1024*10)

#define     BLOCK_GUESTOS		0
#define     BLOCK_SHELLCODE		1
#define     BLOCK_MINIDUMP		2

#define     PAGE_SIZE			 4096
#define     SIZE_TO_PAGES(size)	 ((uint32_t)(size + 4095)>>12)
#define     PAGES_TO_SIZE(pages) ((uint32_t)(pages)<<12)
#define     PAGE_ALGIN_SIZE(size)((uint32_t)(size + 4095)&0xFFFFF000)

struct OS_INFO
{
	uint32_t ram_size;
	uint32_t sys_info_size;

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
	uint32_t page_frame_used;
	uint32_t ram_used;
};

//物理页2*256
struct MEM_BLOCK_INFO
{
	uint32_t physical_address;
	uint32_t virtual_address;
	uint32_t size;
	uint32_t flags;
};


