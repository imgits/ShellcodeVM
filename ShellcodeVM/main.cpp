#include "stdafx.h"
#include "WinMiniDump.h"
#include "page_frame.h"
#include "paging.h"

#define GUEST_RAM_SIZE	(1024*1024*100)
#define PAGE_TABLE_BASE	0xC0000000

extern "C" void boot_code_start();
extern "C" void boot_code_end();

pager vm_pager;
page_frame_database vm_page_frame_db;

bool init_guest_os()
{
	uint64  guest_mem = (uint64)malloc(GUEST_RAM_SIZE);
	vm_pager.Init(&vm_page_frame_db, guest_mem, PAGE_TABLE_BASE);
	vm_page_frame_db.Init(&vm_pager, GUEST_RAM_SIZE);
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

void SetCpuState(hax_vcpu_state *Cpu, UINT32 EntryPoint)
{
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_SELECTOR,0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_LIMIT, 0xfffff); //代码位于0x10000以上，因此将CS段限设为1M
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_ACCESS_RIGHTS, 0x9b);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_BASE,0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_DS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_DS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_DS_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_DS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_ES_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_ES_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_ES_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_ES_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_FS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_FS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_FS_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_FS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GS_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_SS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_SS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_SS_ACCESS_RIGHTS, 0x93);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_SS_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_LDTR_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_LDTR_LIMIT, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_LDTR_ACCESS_RIGHTS, 0x10000);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_LDTR_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_TR_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_TR_LIMIT, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_TR_ACCESS_RIGHTS, 0x83);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_TR_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GDTR_LIMIT, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_GDTR_BASE, 0);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_IDTR_LIMIT, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_IDTR_BASE, 0);

	//VCpu_WriteVMCS(Cpu, VMCS_GUEST_CR0, 0x20 | 1);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CR3, 0x0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CR4, 0x2000);

	VCpu_WriteVMCS(Cpu, VMCS_GUEST_RSP, 0x100);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_RIP, EntryPoint);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_RFLAGS, 0x200 | 0x2);
}

void Run(hax_state *State, hax_vcpu_state *Cpu, uint64 guest_ram)
{
	hax_populate_ram(State->vm, guest_ram, GUEST_RAM_SIZE);
	hax_set_phys_mem(State, 0, GUEST_RAM_SIZE, guest_ram);

	VCpu_Run(Cpu);

	vcpu_state_t state;
	memset(&state, 0, sizeof(vcpu_state_t));

	if (hax_sync_vcpu_state(Cpu, &state, 0) < 0)
		__debugbreak();
}

uint32 ReadOsLoader(char* filename, byte* guest_ram)
{
	byte buf[0x10000];
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		printf("Open %s failed\n",filename);
		return 0;
	}
	int fsize = fread(buf, 1, sizeof(buf), fp);
	fclose(fp);
	PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)buf;
	PIMAGE_NT_HEADERS pe_hdr = (PIMAGE_NT_HEADERS)(buf + dos_hdr->e_lfanew);
	uint32 ImageBase = pe_hdr->OptionalHeader.ImageBase;
	uint32 ImageSize = pe_hdr->OptionalHeader.SizeOfImage;
	uint32 EntryPoint= pe_hdr->OptionalHeader.AddressOfEntryPoint;
	memset(buf + fsize, 0, ImageSize - fsize);
	memcpy(guest_ram + ImageBase, buf, ImageSize);
	return ImageBase + EntryPoint;
}


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
	
	// Is everything loaded and compatible?
	if (!VM_HaxEnabled())	return 1;

	// Create the VM instance with 16kb RAM
	auto state = VM_CreateInstance(GUEST_RAM_SIZE);

	if (!state)	return 1;
	// Set up the virtual processor
	auto cpu = VCpu_Create(state);

	if (!cpu) return 1;

	VCpu_Init(cpu);
	byte* guest_ram = (byte*)VirtualAlloc(nullptr, GUEST_RAM_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	uint32 OsLoader_EntryPoint = ReadOsLoader(argv[1], guest_ram);
	if (OsLoader_EntryPoint != 0)
	{
		SetCpuState(cpu, OsLoader_EntryPoint);
		Run(state, cpu, (UINT64)guest_ram);
	}
	//init_guest_os();
	//dump_vcpu_state(cpu);

	//TestDOS(state, cpu);

	VCpu_Destroy(state, cpu);
	VM_DestroyInstance(state);

	getchar();
	return 0;
}