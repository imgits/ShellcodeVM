#pragma once
#include "typedef.h"
#include "gdt.h"
#define  TSS_BASE_SIZE	103
class TSS
{
	uint32	m_back_link;
	uint32	m_esp0;	//kernel esp
	uint32	m_ss0;
	uint32	m_esp1;
	uint32	m_ss1;
	uint32	m_esp2;
	uint32	m_ss2;
	uint32	m_cr3;
	uint32	m_eip;
	uint32	m_eflags;
	uint32	m_eax, m_ecx, m_edx, m_ebx;
	uint32	m_esp, m_ebp, m_esi, m_edi;
	uint32	m_es, m_cs, m_ss, m_ds, m_fs, m_gs;
	uint32	m_ldt;
	uint16	m_trap;
	uint16	m_iobase;
	// I/O位图基址大于或等于TSS段界限，就表示没有I/O许可位图 
	//8192KB包含64K个端口
	//uint8		m_iomap[8192];
private:
	uint16   m_tss_seg;
public:
	TSS();
	~TSS();
	void Init(GDT * gdt);

public:
	void active();

};
