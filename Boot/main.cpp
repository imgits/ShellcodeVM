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

VM_CONTEXT*  vm = (VM_CONTEXT*)VM_CONTEXT_BASE;

extern "C"  int liballoc_lock()		{	return 0;}
extern "C"  int liballoc_unlock()	{	return 0;}
extern "C"  void* liballoc_alloc(int pages) { return 0; }
extern "C"  int liballoc_free(void*ptr, int size){return 0;}

void main()
{
	CppInit();
	video.Init(vm->video_buf_base, vm->video_buf_size, 100, 100);
	video.puts("hello world\n");
	uint32_t buf_base = vm->video_buf_base;
	uint32_t buf_size = vm->video_buf_size;

	__asm mov eax, buf_base
	__asm mov ebx, buf_size

	gdt.Init();
	idt.Init();

	pager.Init(SIZE_TO_PAGES(vm->ram_used), SIZE_TO_PAGES(vm->ram_size));
	__asm mov eax,0x12345678
	__asm mov ebx,0x87654321
	__asm hlt
	__asm jmp $
}