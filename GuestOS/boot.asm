
        ORG     0x7C00
		BITS    16
        SECTION .text
 start:
		jmp		boot_code16 ;5 byte
		nop
guest_loader_main dd 0
boot_code16:
;------------------------------------------------------------------
;初始化全局段描述符表，为进入保护模式作准备
		 cli      ;此处关闭中断非常重要,否则，VMware中执行lidt	[IDTR]时会产生异常
		 lgdt	[GDTR]
		 lidt	[IDTR]
;-------------------------------------------------------------------
;启动保护模式
         mov		eax,cr0                  
         or			eax,1
         mov		cr0,eax                        ;设置PE位

         ;以下进入保护模式... ...
         jmp       dword GDT32.code32:boot_code32 ;16位的描述符选择子：32位偏移
                                                 ;清流水线并串行化处理器
;===============================================================================
        [bits 32]               
   boot_code32:                                  
         mov		ax,		GDT32.data32                    ;加载数据段(4GB)选择子
         mov		ds,		ax
         mov		es,		ax
         mov		fs,		ax
         mov		gs,		ax
         mov		ss,		ax										;加载堆栈段(4GB)选择子
		 mov        esp,    0x10000
		 mov        eax,    guest_loader_main
		 call       eax

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


