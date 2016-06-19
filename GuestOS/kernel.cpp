#include <stdint.h>
#include <stdio.h>
#include "c++.h"
#include "os.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"
#include "trap.h"
#include "paging.h"
#include "video.h"
#include "Shellcode.h"

GDT		gdt;
IDT		idt;
TSS     tss;
VIDEO   video;
PAGER	pager;
Shellcode shellcode;

extern "C"  int liballoc_lock() { return 0; }
extern "C"  int liballoc_unlock() { return 0; }
extern "C"  void* liballoc_alloc(int pages) { return 0; }
extern "C"  int liballoc_free(void*ptr, int size) { return 0; }

void return_to_user_mode()
{
	__asm cli
	__asm xor   eax,eax
	__asm mov   ax, SEL_USER_DATA
	__asm mov   ds, ax
	__asm mov   es, ax
	__asm mov   gs, ax

	__asm mov   ax, SEL_USER_FS
	__asm mov   fs, ax

	__asm push  SEL_USER_DATA //Ring3_SS
	__asm push  SHELLCODE_STACK_VIRTUAL_ADDR//Ring3_ESP
	__asm pushfd
	__asm push  SEL_USER_CODE //Ring3_CS
	__asm push  SHELLCODE_BUF_VIRTUAL_ADDR//Ring3_EIP
	__asm iretd
}

int  main()
{
	CppInit();
	video.Init(VIDEO_BUF_VIRTUAL_ADDR, MB(2), 100, 100);
	gdt.Init();
	idt.Init();
	tss.Init(&gdt);
	TRAP::Init(&idt);
	tss.active();
	__asm mov esp,	KERNEL_STACK_TOP_VIRTUAL_ADDR 
	pager.unmap_pages(0, FRAME_DB_PHYSICAL_ADDR);
	//pager.map_pages(SHELLCODE_TEB_PHYSICAL_ADDR, SHELLCODE_TEB_VIRTUAL_ADDR, SHELLCODE_TEB_SIZE, PT_READONLY | PT_USER);
	//pager.map_pages(DUMMY_PEB_PHYSICAL_ADDR, DUMMY_PEB_VIRTUAL_ADDR, DUMMY_PEB_SIZE,PT_READONLY | PT_USER);
	//pager.map_pages(KERNEL32_PHYSICAL_ADDR, KERNEL32_VIRTUAL_ADDR, KERNEL32_SIZE, PT_READONLY | PT_USER);
	pager.map_pages(SHELLCODE_BUF_PHYSICAL_ADDR, SHELLCODE_BUF_VIRTUAL_ADDR, SHELLCODE_BUF_SIZE, PT_PRESENT | PT_READONLY | PT_USER);
	
	UINT32 test_code_buf = 0;
	UINT32 test_code_len = 0;
	__asm jmp test_code_end
	{
	__asm test_code_start:
	{
	__asm MOV EAX, 0x11111111
	__asm MOV EBX, 0x22222222
	__asm MOV ECX, 0x33333333
	__asm MOV EDX, 0x44444444
	__asm hlt
	}
	__asm test_code_end:
	}
	__asm mov eax, test_code_start
	__asm mov test_code_buf, eax
	__asm mov eax, test_code_end
	__asm SUB eax, test_code_start
	__asm mov test_code_len, eax

	memcpy((byte*)SHELLCODE_BUF_VIRTUAL_ADDR, (void*)test_code_buf, test_code_len);

	return_to_user_mode();
	//video.puts("Hello world\n");
	__asm mov edx,0x44444444
	__asm mov ecx,0
	__asm div ecx

	__asm jmp $
	return 0;
}