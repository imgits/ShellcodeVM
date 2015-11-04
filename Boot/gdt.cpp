#include "gdt.h"
#include "idt.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

//http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
GDT::GDT()
{
	memset(&m_gdt, 0, sizeof(m_gdt));
}

GDT::~GDT()
{
}

void GDT::Init()
{
	__asm cli

	// 初始化一个代码段和一个数据段，内核态的线程使用
	set_gdt_entry(GDT_KERNEL_CODE, 0x00000000, 0x000FFFFF, DA_CR | DA_32 | DA_DPL0 | DA_LIMIT_4K); //
	set_gdt_entry(GDT_KERNEL_DATA, 0x00000000, 0x000FFFFF, DA_DRW | DA_32 | DA_DPL0 | DA_LIMIT_4K);

	// 用户态线程使用
	set_gdt_entry(GDT_USER_CODE, 0x00000000, 0x0007FFFF, DA_CR | DA_32 | DA_DPL3 | DA_LIMIT_4K);
	set_gdt_entry(GDT_USER_DATA, 0x00000000, 0x0007FFFF, DA_DRW | DA_32 | DA_DPL3 | DA_LIMIT_4K);

	set_gdt_entry(GDT_KERNEL_FS, 0xFFDFF000, 0x000003FF, DA_DRW | DA_32 | DA_DPL3);
	set_gdt_entry(GDT_USER_FS,   0x7EFDD000, 0x000003FF, DA_DRW | DA_32 | DA_DPL3);
	//set_gdt_entry(GDT_USER_FS+1, 0x00010000, 0x0000FFFF, DA_CR  | DA_16 | DA_DPL0);
	//set_gdt_entry(GDT_USER_FS+2, 0x00010000, 0x0000FFFF, DA_DRW | DA_16 | DA_DPL0);

	GDTR gdtr;

	gdtr.limit = sizeof(m_gdt) - 1;
	gdtr.base = m_gdt;
	__asm 
	{
		lgdt gdtr
			mov  eax, SEL_KERNEL_DATA
			mov  ds, ax
			mov  es, ax
			mov  fs, ax
			mov  gs, ax
			mov  ss, ax

			push SEL_KERNEL_CODE
			push flush_cs //jmp  0x08:flush_cs
			retf
			flush_cs :
	}
	//printf("GDT::Init() OK\n");
}

//设置Global Descriptor Table项
bool GDT::set_gdt_entry(uint32_t vector, uint32_t base, uint32_t limit, uint32_t attribute)
{
	if (vector <= 0 || vector >= MAX_GDT_NUM)
	{
		return false;
	}
	SEGMENT_DESC* desc = m_gdt + vector;
	desc->limit_low = limit & 0x0FFFF;
	desc->base_low = base & 0x0FFFF;
	desc->base_mid = (base >> 16) & 0x0FF;
	desc->attr1 = attribute & 0xFF;
	desc->limit_high_attr2 = ((limit >> 16) & 0x0F) | ((attribute >> 8) & 0xF0);
	desc->base_high = (base >> 24) & 0x0FF;
	return true;
}

//添加Global Descriptor Table项
uint32_t  GDT::add_gdt_entry(uint32_t base, uint32_t limit, uint32_t attribute)
{
	int vector = find_free_gdt_entry();
	if (vector == 0) return 0;
	SEGMENT_DESC* desc = m_gdt + vector;

	desc->limit_low = limit & 0x0FFFF;
	desc->base_low = base & 0x0FFFF;
	desc->base_mid = (base >> 16) & 0x0FF;
	desc->attr1 = attribute & 0xFF;
	desc->limit_high_attr2 = ((limit >> 16) & 0x0F) | ((attribute >> 8) & 0xF0);
	desc->base_high = (base >> 24) & 0x0FF;
	return vector * 8;
}

//设置Local Descriptor Table项
uint32_t  GDT::add_ldt_entry(uint32_t base, uint32_t limit, uint32_t attribute)
{
	int vector = find_free_gdt_entry();
	if (vector == 0) return 0;
	SEGMENT_DESC* desc = m_gdt + vector;

	desc->limit_low = limit & 0x0FFFF;
	desc->base_low = base & 0x0FFFF;
	desc->base_mid = (base >> 16) & 0x0FF;
	desc->attr1 = (attribute & 0xFF);
	desc->limit_high_attr2 = ((limit >> 16) & 0x0F) | ((attribute >> 8) & 0xF0);
	desc->base_high = (base >> 24) & 0x0FF;
	return vector * 8;
}

//添加TSS项
uint32_t GDT::add_tss_entry(uint32_t base, uint32_t limit)
{
	int vector = find_free_gdt_entry();
	if (vector == 0) return 0;
	SEGMENT_DESC* desc = m_gdt + vector;

	desc->limit_low = limit & 0x0FFFF;
	desc->base_low = base & 0x0FFFF;
	desc->base_mid = (base >> 16) & 0x0FF;
	desc->attr1 = DA_386TSS;
	desc->limit_high_attr2 = ((limit >> 16) & 0x0F);
	desc->base_high = (base >> 24) & 0x0FF;
	return vector * 8;
}

//添加系统调用门
uint32_t GDT::add_call_gate(void* handler, int argc)
{
	int vector = find_free_gdt_entry();
	if (vector == 0) return 0;
	GATE_DESC* desc = (GATE_DESC*)(m_gdt + vector);

	uint32_t base = (uint32_t)handler;

	desc->offset_low = base & 0xFFFF;
	desc->selector = SEL_KERNEL_CODE;	//系统代码段
	desc->dcount = argc;
	desc->attr = DA_386CGate | DA_DPL3;
	desc->offset_high = (base >> 16) & 0xFFFF;
	return vector * 8;
}

uint32_t GDT::find_free_gdt_entry()
{
	int vector = 0;
	uint64_t* pgdt = (uint64_t*)m_gdt;
	for (int i = 1; i < MAX_GDT_NUM; i++)
	{
		if (pgdt[i] == 0) return i;
	}
	return 0;
}