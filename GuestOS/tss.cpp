#include "tss.h"
#include <stdio.h>
#include <string.h>
#include <intrin.h>
#include "os.h"

TSS::TSS()
{
	//在程序管理器的TSS中设置必要的项目 
	memset(this, 0, sizeof(TSS)); 
}

TSS::~TSS()
{
}

void TSS::Init(GDT * gdt)
{
	//在程序管理器的TSS中设置必要的项目 
	memset(this, 0, sizeof(TSS)); 

	m_back_link = 0;//反向链=0
	m_esp0 = KERNEL_STACK_TOP_VIRTUAL_ADDR;
	m_ss0 = SEL_KERNEL_DATA;
	m_cr3 = __readcr3();//登记CR3(PDBR)
	m_eip = 0;//(uint32)UserProcess;
	m_eflags = 0x00000202;

	m_ldt = 0;//没有LDT。处理器允许没有LDT的任务
	m_trap = 0;
	m_iobase = 104;//没有I/O位图。0特权级事实上不需要。
				   //创建程序管理器的TSS描述符，并安装到GDT中 
	m_tss_seg = gdt->add_tss_entry((uint32)this, 103);
}

void TSS::active()
{
	//任务寄存器TR中的内容是任务存在的标志，该内容也决定了当前任务是谁。
	//下面的指令为当前正在执行的0特权级任务“程序管理器”后补手续（TSS）。
	__asm	mov		ecx,	this
	__asm	mov		ax,		[ecx]TSS.m_tss_seg //index = 5
	__asm	ltr		ax
	//printf("TSS Register() OK\n");
}