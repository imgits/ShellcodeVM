#include "stdafx.h"
#include "WinMiniDump.h"
#include "page_frame.h"
#include "paging.h"
#include "os.h"
#include "HAXM.h"
#include "msr.h"

bool   LoadMiniDump(char* filename, VM_CONTEXT* vm)
{
	WinMiniDump MiniDump;
	if (!MiniDump.Open(filename)) return false;
	PMINIDUMP_MEMORY64_LIST Mem64List = (PMINIDUMP_MEMORY64_LIST)MiniDump.GetStream(Memory64ListStream);
	PMINIDUMP_MEMORY_DESCRIPTOR64 mem_desc = Mem64List->MemoryRanges;
	
	byte*   DataBase = (byte*)MiniDump.RvaToAddress(Mem64List->BaseRva);
	vm->mem_blocks =Mem64List->NumberOfMemoryRanges;
	
	MEM_BLOCK_INFO* mem_block_info = (MEM_BLOCK_INFO*)(vm->ram_base + vm->ram_used);
	uint32_t mem_block_list_size = PAGE_ALGIN_SIZE(vm->mem_blocks * sizeof(MEM_BLOCK_INFO));
	uint32_t mem_block_data_offset = vm->ram_used + mem_block_list_size;
	uint32_t  mem_block_total_size = mem_block_list_size;
	for (int i = 0; i < Mem64List->NumberOfMemoryRanges; i++, mem_desc++)
	{
		mem_block_info[i].physical_address = mem_block_data_offset;
		mem_block_info[i].virtual_address = mem_desc->StartOfMemoryRange;
		mem_block_info[i].size = mem_desc->DataSize;
		mem_block_info[i].flags = BLOCK_MINIDUMP;

		memcpy((byte*)vm->ram_base + mem_block_data_offset, DataBase, mem_desc->DataSize);

		DataBase += mem_desc->DataSize;
		mem_block_data_offset += mem_desc->DataSize;
		mem_block_total_size += mem_desc->DataSize;
	}
	vm->ram_used += mem_block_total_size;
	return true;
}

bool   ReserveShellcodeMemory(VM_CONTEXT* vm)
{
	vm->shellcode_base = vm->ram_used;
	vm->shellcode_buf_size = SHELLCODE_BUF_SIZE;
	vm->shellcode_file_size = 0;
	vm->ram_used += SHELLCODE_BUF_SIZE;
	return true;
}

bool	LoadBootCode(char* filename,VM_CONTEXT* vm)
{
	byte buf[0x10000];
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		printf("Open %s failed\n",filename);
		return false;
	}
	int fsize = fread(buf, 1, sizeof(buf), fp);
	fclose(fp);
	PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)buf;
	PIMAGE_NT_HEADERS pe_hdr = (PIMAGE_NT_HEADERS)(buf + dos_hdr->e_lfanew);
	uint32 ImageBase = pe_hdr->OptionalHeader.ImageBase;
	uint32 ImageSize = pe_hdr->OptionalHeader.SizeOfImage;
	uint32 EntryPoint= pe_hdr->OptionalHeader.AddressOfEntryPoint;
	memset(buf + fsize, 0, ImageSize - fsize);
	memcpy((byte*)vm->ram_base + ImageBase, buf, ImageSize);
	vm->boot_start = ImageBase + EntryPoint;
	return true;
}

bool    LoadGuestOs(char* filename, VM_CONTEXT* vm)
{
	FILE* fp = fopen(filename, "rb");
	if (fp == NULL) return false;
	fseek(fp, 0, SEEK_END);
	uint32_t fsize = ftell(fp);
	rewind(fp);
	byte*  os_buf = (byte*)vm->ram_base + OS_CODE_BASE;
	if (fread(os_buf, 1, fsize, fp) != fsize)
	{
		fclose(fp);
		return false;
	}

	PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)os_buf;
	PIMAGE_NT_HEADERS pe_hdr = (PIMAGE_NT_HEADERS)(os_buf + dos_hdr->e_lfanew);

	uint32_t os_image_base = pe_hdr->OptionalHeader.ImageBase;
	uint32_t os_image_size = pe_hdr->OptionalHeader.SizeOfImage;
	uint32_t os_entry_point = pe_hdr->OptionalHeader.AddressOfEntryPoint;

	//初始化os_info
	vm->os_physcial_address = OS_CODE_BASE;
	vm->os_virtual_address = os_image_base;
	vm->os_image_size = os_image_size;
	vm->os_entry_point = os_image_base + os_entry_point;

	vm->ram_used = OS_CODE_BASE + PAGE_ALGIN_SIZE(os_image_size);

	return true;
}

bool	CreateMiniDump()
{
	WinMiniDump MiniDump;
	return MiniDump.Create(
		"MiniDump.Dmp", 
		NULL, 
		0,
		(MINIDUMP_TYPE)(
			(UINT32)MiniDumpWithFullMemory |
			(UINT32)MiniDumpWithFullMemoryInfo |
			(UINT32)MiniDumpWithHandleData |
			(UINT32)MiniDumpWithThreadInfo |
			(UINT32)MiniDumpWithUnloadedModules
			)
		);
}

bool	PrepareVmContext(int argc, char *argv[],VM_CONTEXT * vm)
{
	memset(vm, 0, sizeof(VM_CONTEXT));
	vm->ram_base = (UINT32)VirtualAlloc(nullptr, VM_RAM_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (vm->ram_base == NULL)
	{
		printf("Allocate VM RAM failed\n");
		return false;
	}
	vm->ram_size = VM_RAM_SIZE;
	vm->video_buf_base = VIDEO_BUF_BASE;
	vm->video_buf_size = VIDEO_BUF_SIZE;
	if (!LoadBootCode(argv[1], vm)) return false;
	if (!LoadGuestOs(argv[2], vm)) return false;
	if (!LoadMiniDump(argv[3], vm)) return false;
	ReserveShellcodeMemory(vm);

	memcpy((byte*)vm->ram_base + VM_CONTEXT_BASE, vm, sizeof(VM_CONTEXT));
	return true;
}

//物理页4k-64k 为启动代码
int main(int argc, char *argv[])
{
	VM_CONTEXT vm_context;
	if (!PrepareVmContext(argc, argv, &vm_context))
	{
		return -1;
	}

	HAXM haxm;
	HAXM_VM*  vm = NULL;
	HAXM_CPU* cpu = NULL;
	while (
		haxm.open_device() &&
		haxm.is_available() &&
		haxm.is_supported()
		)
	{
		if (!(vm = haxm.create_vm())) break;
		if (!vm->hax_populate_ram(vm_context.ram_base, VM_RAM_SIZE)) break;
		if (!vm->hax_set_phys_mem(0, VM_RAM_SIZE, vm_context.ram_base)) break;
		
		if (!(cpu = vm->create_cpu())) break;
		if (!cpu->get_cpu_state()) break;
		cpu->show_state();

		hax_msr_data msrs;
		memset(&msrs, 0, sizeof(msrs));
		msrs.entries[0].entry = IA32_FEATURE_CONTROL_CODE;
		msrs.entries[1].entry = IA32_SYSENTER_CS;
		msrs.entries[2].entry = IA32_SYSENTER_ESP;
		msrs.entries[3].entry = IA32_SYSENTER_EIP;
		msrs.entries[4].entry = IA32_DEBUGCTL;
		msrs.entries[5].entry = IA32_VMX_BASIC_MSR_CODE;
		msrs.entries[6].entry = IA32_VMX_PINBASED_CTLS;
		msrs.entries[7].entry = IA32_VMX_PROCBASED_CTLS;
		msrs.entries[8].entry = IA32_VMX_EXIT_CTLS;
		msrs.entries[9].entry = IA32_VMX_ENTRY_CTLS;
		msrs.entries[10].entry = IA32_VMX_MISC;
		msrs.entries[11].entry = IA32_VMX_CR0_FIXED0;
		msrs.entries[12].entry = IA32_VMX_CR0_FIXED1;
		msrs.entries[13].entry = IA32_VMX_CR4_FIXED0;
		msrs.entries[14].entry = IA32_VMX_CR4_FIXED1;
		msrs.entries[15].entry = IA32_FS_BASE;
		msrs.entries[16].entry = IA32_GS_BASE;

		msrs.entries[17].entry = MSR_LASTBRANCH_0_FROM_IP;
		msrs.entries[18].entry = MSR_LASTBRANCH_0_TO_IP;
		msrs.entries[19].entry = MSR_LASTBRANCH_TOS;
		msrs.nr_msr = 20;
		cpu->get_msrs(&msrs);

		cpu->reset(vm_context.boot_start);
		cpu->show_state();
		char* dgbinfo = (char*)(vm_context.ram_base + vm_context.video_buf_base);
		int count = 0;
		int exit_status = 0;
		do
		{
			exit_status = cpu->run();
			cpu->get_cpu_state();
			cpu->show_state();
			switch (exit_status)
			{
				// Regular I/O
			case HAX_EXIT_IO:
				printf("HAX_EXIT_IO\n");
				break;

				// Memory-mapped I/O
			case HAX_EXIT_MMIO:
				printf("HAX_EXIT_MMIO\n");
				break;

				// Fast memory-mapped I/O
			case HAX_EXIT_FAST_MMIO:
				printf("HAX_EXIT_FAST_MMIO\n");
				break;

				// Guest running in real mode
			case HAX_EXIT_REAL:
				printf("HAX_EXIT_REAL\n");
				break;

				// Guest state changed, currently only for shutdown
			case HAX_EXIT_STATECHANGE:
				printf("VCPU shutdown request\n");
				//exit_status = 0;
				break;

			case HAX_EXIT_UNKNOWN_VMEXIT:
				dprint("Unknown VMX exit %x from guest\n", exit_status);
				exit_status = 0;
				break;

				// HLT instruction executed
			case HAX_EXIT_HLT:
				printf("HAX_EXIT_HLT\n");
				break;

				// these situations will continue to the Hax module
			case HAX_EXIT_INTERRUPT:
				printf("HAX_EXIT_INTERRUPT\n");
				break;
			case HAX_EXIT_PAUSED:
				printf("HAX_EXIT_PAUSED\n");
				break;

			default:
				printf("Unknown exit %x from Hax driver\n", exit_status);
				exit_status = 0;
				break;
			}
		}while (count++ < 10 && exit_status > 0);

		break;
	}
	VirtualFree((void*)vm_context.ram_base, 0, MEM_RELEASE);

	getchar();
	return 0;
}