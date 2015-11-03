%define CODE_BASE						        0x00000000
%define REBASE(addr)                           (CODE_BASE + (addr - boot_code_start))

[bits 32]
global boot_code_start,_boot_code_start,boot_code_end,_boot_code_end

        ;ORG     0x0
        BITS    16
        SECTION .text
 boot_code_start:
_boot_code_start:
		mov		ax,0x01
		mov		bx,0x02
		mov		cx,0x03
		mov		dx,0x04
		mov		sp,0x05
		mov		bp,0x06
		mov		si,0x07
		mov		di,0x08
		hlt
;------------------------------------------------------------------
;初始化全局段描述符表，为进入保护模式作准备
		 cli      ;此处关闭中断非常重要,否则，VMware中执行lidt	[IDTR]时会产生异常
		 lgdt	[0x1020]
		 lidt	[0x1030]

;-------------------------------------------------------------------
;启动保护模式
         mov		eax,cr0                  
		 mov        ebx,eax
         or			eax,1
         mov		cr0,eax                        ;设置PE位
         mov        ecx,cr0

         ;以下进入保护模式... ...
         jmp       dword 0x08:REBASE(boot_code32) ;16位的描述符选择子：32位偏移
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

		mov		eax,0x11111111
		mov		ebx,0x22222222
		mov		ecx,0x33333333
		mov		edx,0x44444444
		mov		esp,0x55555555
		mov		ebp,0x66666666
		mov		esi,0x77777777
		mov		edi,0x88888888
		hlt

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

