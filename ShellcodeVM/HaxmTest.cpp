#include "stdafx.h"
#include "WinMiniDump.h"

extern "C" void boot_code_start();
extern "C" void boot_code_end();

void TestDOS(hax_state *State, hax_vcpu_state *Cpu)
{
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_SELECTOR, 0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_LIMIT, 0xffff);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_ACCESS_RIGHTS, 0x9b);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_CS_BASE, 0);

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
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_RIP, 0x0);
	VCpu_WriteVMCS(Cpu, VMCS_GUEST_RFLAGS, 0x200 | 0x2);

	LPVOID mem = VirtualAlloc(nullptr, 16384, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memset(mem, 0x90, 16384);
	uint32 code_start = (uint32)boot_code_start;
	uint32 code_end   = (uint32)boot_code_end;
	uint32 code_size = code_end - code_start;
	memcpy(mem, boot_code_start, code_size);

	*(uint64*)((uint64)mem + 0x1000) = 0; //null
	*(uint64*)((uint64)mem + 0x1008) = 0x00cf9A000000ffff;//code32
	*(uint64*)((uint64)mem + 0x1010) = 0x00cf92000000ffff;//data32
	
	*(uint16*)((uint64)mem + 0x1020) = 3*8-1;//GDTR.limit
	*(uint32*)((uint64)mem + 0x1022) = 0x1000;//GDTR.base

	*(uint16*)((uint64)mem + 0x1030) = 0;//IDTR.limit
	*(uint32*)((uint64)mem + 0x1032) = 0;//IDTR.base

	hax_populate_ram(State->vm, (uint64)mem, 16384);
	hax_set_phys_mem(State, 0, 16384, (uint64)mem);

	VCpu_Run(Cpu);

	vcpu_state_t state;
	memset(&state, 0, sizeof(vcpu_state_t));

	if (hax_sync_vcpu_state(Cpu, &state, 0) < 0)
		__debugbreak();
}


void dump_vcpu_state(hax_vcpu_state *CPU);
int main(int argc, char *argv[])
{
	WinMiniDump MiniDump;
	//MiniDump.Create("MiniDump.Dmp");
	MiniDump.Open("MiniDump.Dmp");
	MiniDump.DumpHeader();
	MiniDump.DumpDirectory();

	// Is everything loaded and compatible?
	if (!VM_HaxEnabled())
		return 1;

	// Create the VM instance with 16kb RAM
	auto state = VM_CreateInstance(16384);

	if (!state)
		return 1;

	// Set up the virtual processor
	auto cpu = VCpu_Create(state);

	if (!cpu)
		return 1;

	VCpu_Init(cpu);
	dump_vcpu_state(cpu);
	TestDOS(state, cpu);

	getchar();
	VCpu_Destroy(state, cpu);
	VM_DestroyInstance(state);

	return 0;
}