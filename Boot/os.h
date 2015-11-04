#pragma once
#include <stdint.h>

//物理页0(空闲)

//物理页1
struct SYSTEM_INFO
{
	uint32_t os_physcial_address;
	uint32_t os_virtual_address;
	uint32_t os_image_size;
	uint32_t os_entry_point;

	uint32_t shellcode_physcial_address;
	uint32_t shellcode_virtual_address;
	uint32_t shellcode_buffer_size;
	uint32_t shellcode_stack_size;

	uint32_t ram_size;
	uint32_t sys_info_size;

	uint32_t kernel_code, kernel_data, user_code, user_data;
	uint32_t fs, gs;
	uint32_t mem_blocks;
	uint32_t next_free_page_frame;
};

//物理页2*256
struct MEM_BLOCK_INFO
{
	uint32_t physical_address;
	uint32_t virtual_address;
	uint32_t size;
	uint32_t flags;
};

