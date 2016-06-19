#include "stdafx.h"
#include "WinMiniDump.h"
#include "page_frame.h"
#include "paging.h"
#include "os.h"
#include "HAXM.h"
#include "msr.h"
#include "Shellcode.h"

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
		printf("%4d %08X %08X\n", i, (UINT32)mem_desc->StartOfMemoryRange, (UINT32)mem_desc->DataSize);
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
	byte*  os_buf = (byte*)vm->ram_base + OS_CODE_PHYSICAL_ADDR;
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
	vm->os_physcial_address = OS_CODE_PHYSICAL_ADDR;
	vm->os_virtual_address = os_image_base;
	vm->os_image_size = os_image_size;
	vm->os_entry_point = os_image_base + os_entry_point;

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
	vm->video_buf_base = VIDEO_BUF_PHYSICAL_ADDR;
	vm->video_buf_size = VIDEO_BUF_SIZE;
	if (!LoadBootCode(argv[1], vm)) return false;
	if (!LoadGuestOs(argv[2], vm)) return false;
	//if (!LoadMiniDump(argv[3], vm)) return false;
	//ReserveShellcodeMemory(vm);
	memcpy((byte*)vm->ram_base + SHELLCODE_TEB_PHYSICAL_ADDR, (void*)SHELLCODE_TEB_VIRTUAL_ADDR, SHELLCODE_TEB_SIZE);
	memcpy((byte*)vm->ram_base + VM_CONTEXT_PHYSICAL_ADDR, vm, sizeof(VM_CONTEXT));

	UINT32 Kernel32Base = (UINT32)GetModuleHandle("Kernel32");
	PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)Kernel32Base;
	PIMAGE_NT_HEADERS pe_hdr = (PIMAGE_NT_HEADERS)(Kernel32Base + dos_hdr->e_lfanew);

	uint32_t image_base = pe_hdr->OptionalHeader.ImageBase;
	uint32_t image_size = pe_hdr->OptionalHeader.SizeOfImage;
	image_size = 0x000D0000;
	memcpy((byte*)vm->ram_base + KERNEL32_PHYSICAL_ADDR, (void*)Kernel32Base, image_size);


	return true;
}

#define HAX_IOCTL_CODE(code) printf(#code"=%08X %u\n",code, code);
void ShowIoCtrlCode()
{
	HAX_IOCTL_CODE(HAX_IOCTL_VERSION);
	HAX_IOCTL_CODE(HAX_IOCTL_CREATE_VM);
	HAX_IOCTL_CODE(HAX_IOCTL_CAPABILITY);
	HAX_IOCTL_CODE(HAX_VM_IOCTL_VCPU_CREATE);
	HAX_IOCTL_CODE(HAX_VM_IOCTL_ALLOC_RAM);
	HAX_IOCTL_CODE(HAX_VM_IOCTL_SET_RAM);
	HAX_IOCTL_CODE(HAX_VCPU_IOCTL_RUN);
	HAX_IOCTL_CODE(HAX_VCPU_IOCTL_SET_MSRS);
	HAX_IOCTL_CODE(HAX_VCPU_IOCTL_GET_MSRS);

	HAX_IOCTL_CODE(HAX_VCPU_IOCTL_SET_FPU);
	HAX_IOCTL_CODE(HAX_VCPU_IOCTL_GET_FPU);
	HAX_IOCTL_CODE(HAX_VCPU_IOCTL_SETUP_TUNNEL);
	HAX_IOCTL_CODE(HAX_VCPU_IOCTL_INTERRUPT);
	HAX_IOCTL_CODE(HAX_VCPU_SET_REGS);
	HAX_IOCTL_CODE(HAX_VCPU_GET_REGS);
	HAX_IOCTL_CODE(HAX_VM_IOCTL_NOTIFY_QEMU_VERSION);
}

//物理页4k-64k 为启动代码
/* See comments for the ioctl in hax-darwin.h */
/*
#define HAX_IOCTL_VERSION       CTL_CODE(HAX_DEVICE_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_CREATE_VM     CTL_CODE(HAX_DEVICE_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_IOCTL_CAPABILITY    CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VM_IOCTL_VCPU_CREATE   CTL_CODE(HAX_DEVICE_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_ALLOC_RAM     CTL_CODE(HAX_DEVICE_TYPE, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VM_IOCTL_SET_RAM       CTL_CODE(HAX_DEVICE_TYPE, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_RUN      CTL_CODE(HAX_DEVICE_TYPE, 0x906, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_SET_MSRS CTL_CODE(HAX_DEVICE_TYPE, 0x907, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_GET_MSRS CTL_CODE(HAX_DEVICE_TYPE, 0x908, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_SET_FPU  CTL_CODE(HAX_DEVICE_TYPE, 0x909, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_GET_FPU  CTL_CODE(HAX_DEVICE_TYPE, 0x90a, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VCPU_IOCTL_SETUP_TUNNEL      CTL_CODE(HAX_DEVICE_TYPE, 0x90b, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_IOCTL_INTERRUPT        CTL_CODE(HAX_DEVICE_TYPE, 0x90c, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_SET_REGS               CTL_CODE(HAX_DEVICE_TYPE, 0x90d, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define HAX_VCPU_GET_REGS               CTL_CODE(HAX_DEVICE_TYPE, 0x90e, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define HAX_VM_IOCTL_NOTIFY_QEMU_VERSION CTL_CODE(HAX_DEVICE_TYPE, 0x910, METHOD_BUFFERED, FILE_ANY_ACCESS)
*/

bool get_msrs(HAXM_CPU* cpu)
{
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
	return true;
}

int main(int argc, char *argv[])
{
	//CreateMiniDump();
	//ShowIoCtrlCode();
	VM_CONTEXT vm_context = { 0 };
	if (!PrepareVmContext(argc, argv, &vm_context))
	{
		//return -1;
	}
	DUMMY_PEB_MODULE_LIST peb_module_list;
	clone_PEB_Module_list(peb_module_list);

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
				if (cpu->m_state._rip == 0x0000000080002F20)
				{
					int a = 0;
				}
				break;
			case HAX_EXIT_PAUSED:
				printf("HAX_EXIT_PAUSED\n");
				break;

			default:
				printf("Unknown exit %x from Hax driver\n", exit_status);
				exit_status = 0;
				break;
			}
		}while (exit_status > 0);

		break;
	}
	VirtualFree((void*)vm_context.ram_base, 0, MEM_RELEASE);

	//getchar();
	return 0;
}