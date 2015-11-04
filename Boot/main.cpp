#include <stdint.h>
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "c++.h"

#include "os.h"

GDT		gdt;
IDT		idt;
PAGER	pager;
SYSTEM_INFO*  sysinfo = (SYSTEM_INFO*)0x1000;

extern "C"  int liballoc_lock()		{	return 0;}
extern "C"  int liballoc_unlock()	{	return 0;}
extern "C"  void* liballoc_alloc(int pages) { return 0; }
extern "C"  int liballoc_free(void*ptr, int size){return 0;}

void main()
{
	CppInit();

	gdt.Init();
	idt.Init();
	pager.Init(sysinfo->next_free_page_frame, SIZE_TO_PAGES(sysinfo->ram_size));
	__asm mov eax,0x12345678
	__asm mov ebx,0x87654321
	__asm hlt
	__asm jmp $
}