#include "stdafx.h"
#include "WinMiniDump.h"
#include "page_frame.h"
#include "paging.h"
#include "os.h"

#define GUEST_RAM_SIZE	(1024*1024*100)
#define PAGE_TABLE_BASE	0xC0000000

byte*			 guest_ram = NULL;
OS_INFO*		 os_info = NULL;

void   SetCpuState(hax_vcpu_state *Cpu, UINT32 EntryPoint)
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

void   Run(hax_state *State, hax_vcpu_state *Cpu, uint64 guest_ram)
{
	hax_populate_ram(State->vm, guest_ram, GUEST_RAM_SIZE);
	hax_set_phys_mem(State, 0, GUEST_RAM_SIZE, guest_ram);

	VCpu_Run(Cpu);

	vcpu_state_t state;
	memset(&state, 0, sizeof(vcpu_state_t));

	if (hax_sync_vcpu_state(Cpu, &state, 0) < 0)
		__debugbreak();
}

bool   LoadMiniDump(char* filename)
{
	WinMiniDump MiniDump;
	if (!MiniDump.Open(filename)) return false;
	PMINIDUMP_MEMORY64_LIST Mem64List = (PMINIDUMP_MEMORY64_LIST)MiniDump.GetStream(Memory64ListStream);
	PMINIDUMP_MEMORY_DESCRIPTOR64 mem_desc = Mem64List->MemoryRanges;
	
	byte*   DataBase = (byte*)MiniDump.RvaToAddress(Mem64List->BaseRva);
	os_info->mem_blocks =Mem64List->NumberOfMemoryRanges;
	
	MEM_BLOCK_INFO* mem_block_info = (MEM_BLOCK_INFO*)(guest_ram + os_info->ram_used);
	uint32_t mem_block_list_size = PAGE_ALGIN_SIZE(os_info->mem_blocks * sizeof(MEM_BLOCK_INFO));
	uint32_t mem_block_data_offset = os_info->ram_used + mem_block_list_size;
	uint32_t  mem_block_total_size = mem_block_list_size;
	for (int i = 0; i < Mem64List->NumberOfMemoryRanges; i++, mem_desc++)
	{
		mem_block_info[i].physical_address = mem_block_data_offset;
		mem_block_info[i].virtual_address = mem_desc->StartOfMemoryRange;
		mem_block_info[i].size = mem_desc->DataSize;
		mem_block_info[i].flags = BLOCK_MINIDUMP;

		memcpy(guest_ram + mem_block_data_offset, DataBase, mem_desc->DataSize);

		DataBase += mem_desc->DataSize;
		mem_block_data_offset += mem_desc->DataSize;
		mem_block_total_size += mem_desc->DataSize;
	}
	os_info->ram_used += mem_block_total_size;
	return true;
}

bool   ReserveShellcodeMemory()
{
	os_info->shellcode_base = os_info->ram_used;
	os_info->shellcode_buf_size = SHELLCODE_BUF_SIZE;
	os_info->shellcode_file_size = 0;
	os_info->ram_used += SHELLCODE_BUF_SIZE;
}

uint32 LoadBootCode(char* filename)
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

bool   LoadGuestOs(char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL) return false;
	fseek(fp, 0, SEEK_END);
	uint32_t fsize = ftell(fp);
	rewind(fp);
	byte*  pbuf = guest_ram + OS_CODE_BASE;
	if (fread(pbuf, 1, fsize, fp) != fsize)
	{
		fclose(fp);
		return false;
	}

	PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)pbuf;
	PIMAGE_NT_HEADERS pe_hdr = (PIMAGE_NT_HEADERS)(pbuf + dos_hdr->e_lfanew);

	os_info->os_physcial_address = OS_CODE_BASE;
	os_info->os_virtual_address = pe_hdr->OptionalHeader.ImageBase;
	os_info->os_image_size = pe_hdr->OptionalHeader.SizeOfImage;
	os_info->os_entry_point = pe_hdr->OptionalHeader.ImageBase + pe_hdr->OptionalHeader.AddressOfEntryPoint;
	os_info->ram_used = OS_CODE_BASE + PAGE_ALGIN_SIZE(os_info->os_image_size);
	return true;
}


//物理页4k-64k 为启动代码
int main(int argc, char *argv[])
{
	//MiniDump.Create("MiniDump.Dmp", NULL, 0,
	//	(MINIDUMP_TYPE)(
	//		(UINT32)MiniDumpWithFullMemory |
	//		(UINT32)MiniDumpWithFullMemoryInfo |
	//		(UINT32)MiniDumpWithHandleData |
	//		(UINT32)MiniDumpWithThreadInfo |
	//		(UINT32)MiniDumpWithUnloadedModules
	//		)
	//	);
	
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
	os_info = (OS_INFO*)(guest_ram + OS_INFO_BASE);
	memset(os_info, 0, sizeof(OS_INFO));
	uint32 OsLoader_EntryPoint = LoadBootCode(argv[1]);
	if (OsLoader_EntryPoint != 0)
	{
		LoadGuestOs(argv[2]);
		LoadMiniDump(argv[3]);
		ReserveShellcodeMemory();

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