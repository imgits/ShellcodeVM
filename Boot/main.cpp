#include <stdint.h>
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "c++.h"
#include "video.h"

#include "os.h"

GDT		gdt;
IDT		idt;
PAGER	pager;
VIDEO   video;

OS_INFO*  osinfo = (OS_INFO*)OS_INFO_BASE;

extern "C"  int liballoc_lock()		{	return 0;}
extern "C"  int liballoc_unlock()	{	return 0;}
extern "C"  void* liballoc_alloc(int pages) { return 0; }
extern "C"  int liballoc_free(void*ptr, int size){return 0;}

void main()
{
	CppInit();
	video.Init(0x20000, 0x10000, 100, 100);
	video.puts("hello world\n");
	gdt.Init();
	idt.Init();
	pager.Init(osinfo->page_frame_used, SIZE_TO_PAGES(osinfo->ram_size));
	__asm mov eax,0x12345678
	__asm mov ebx,0x87654321
	__asm hlt
	__asm jmp $
}