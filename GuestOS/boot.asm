%define  OS_LOADER_BASE			0x00400000
%define  BOOT_STACK_BASE		0x00010000
%define  REBASE16(addr)        ($$ + (addr - boot_code16))
%define  REBASE32(addr)        ($$ + (addr - boot_code16))

[BITS 32]
global boot_code16
extern  _main

[BITS 16]
        SECTION .text

boot_code16:
 osloader_start:
_osloader_start:
;------------------------------------------------------------------
;初始化全局段描述符表，为进入保护模式作准备
		 cli      ;此处关闭中断非常重要,否则，VMware中执行lidt	[IDTR]时会产生异常
		 a32 lgdt	[cs:REBASE16(GDTR)]
		 a32 lidt	[cs:REBASE16(IDTR)]
;-------------------------------------------------------------------
;启动保护模式
         mov		eax,cr0                  
         or			eax,1
         mov		cr0,eax                        ;设置PE位

         ;以下进入保护模式... ...
         jmp       dword GDT32.code32:REBASE32(boot_code32);16位的描述符选择子：32位偏移
                                                    ;清流水线并串行化处理器
;===============================================================================
[BITS 32]               
boot_code32:                                  
         mov		ax,		GDT32.data32                    ;加载数据段(4GB)选择子
         mov		ds,		ax
         mov		es,		ax
         mov		fs,		ax
         mov		gs,		ax
         mov		ss,		ax										;加载堆栈段(4GB)选择子

		 ;设置栈指针
		 mov        esp,    BOOT_STACK_BASE
		 call		_main
		 jmp		$

		 ;通过PE头部获取入口函数地址
		 mov        esi,	OS_LOADER_BASE;esi = PIMAGE_DOS_HEADER  
		 mov		eax,    [esi + 0x3C] ; eax = PIMAGE_NT_HEADERS
		 mov        eax,    [esi + eax + 0x18 + 0x10];eax =  PIMAGE_NT_HEADERS->IMAGE_OPTIONAL_HEADER.AddressOfEntryPoint
		 add        eax,	esi 
		 call       eax
		 jmp        $

align   8
GDT32:
			dq	0x0000000000000000
.code32		equ  $ - GDT32
		    dq	0x00cf9A000000ffff
.data32		equ  $ - GDT32
			dq	0x00cf92000000ffff

GDTR		dw	$ - GDT32 - 1
			dd	GDT32 ;GDT的物理/线性地址

IDTR		dw	0
			dd	0 ;IDT的物理/线性地址

 boot_code_end:
_boot_code_end:


