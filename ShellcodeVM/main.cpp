#include "stdafx.h"
#include "WinMiniDump.h"
#include "page_frame.h"
#include "paging.h"

#define GUEST_MEM_SIZE	(1024*1024*100)
#define PAGE_TABLE_BASE	0xC0000000

extern "C" void boot_code_start();
extern "C" void boot_code_end();

pager vm_pager;
page_frame_database vm_page_frame_db;

bool init_guest_os()
{
	uint64  guest_mem = (uint64)malloc(GUEST_MEM_SIZE);
	vm_pager.Init(&vm_page_frame_db, guest_mem, PAGE_TABLE_BASE);
	vm_page_frame_db.Init(&vm_pager, GUEST_MEM_SIZE);
	return true;
}

//物理页0
struct system_info
{
	uint32 os_physcial_address;
	uint32 os_virtual_address;
	uint32 os_image_size;
	uint32 os_entry_point;

	uint32 shellcode_physcial_address;
	uint32 shellcode_virtual_address;
	uint32 shellcode_buffer_size;
	uint32 shellcode_stack_size;

	uint32 ram_size;
	uint32 sys_info_size;

	uint32 kernel_code, kernel_data, user_code, user_data;
	uint32 fs, gs;
	uint32 mem_blocks;
	uint32 next_free_page_frame;
};

//物理页1*256
struct mem_block_info
{
	uint32 physical_address;
	uint32 virtual_address;
	uint32 size;
	uint32 flags;
};

//物理页4k-64k 为启动代码
int main(int argc, char *argv[])
{
	WinMiniDump MiniDump;
	MiniDump.Create("MiniDump.Dmp", NULL, 0,
		(MINIDUMP_TYPE)(
			(UINT32)MiniDumpWithFullMemory |
			(UINT32)MiniDumpWithFullMemoryInfo |
			(UINT32)MiniDumpWithHandleData |
			(UINT32)MiniDumpWithThreadInfo |
			(UINT32)MiniDumpWithUnloadedModules
			)
		);
	MiniDump.Open("MiniDump.Dmp");
	MiniDump.DumpHeader();
	MiniDump.DumpDirectory();
	return 0;
	// Is everything loaded and compatible?
	if (!VM_HaxEnabled())	return 1;

	// Create the VM instance with 16kb RAM
	auto state = VM_CreateInstance(16384);

	if (!state)	return 1;
	// Set up the virtual processor
	auto cpu = VCpu_Create(state);

	if (!cpu) return 1;

	VCpu_Init(cpu);

	init_guest_os();
	//dump_vcpu_state(cpu);

	//TestDOS(state, cpu);

	getchar();
	VCpu_Destroy(state, cpu);
	VM_DestroyInstance(state);

	return 0;
}