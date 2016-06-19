#include <trap.h>
#include <stdio.h>
#include <string.h>
#include "os.h"
#include "video.h"

#define TRAP_WITH_ERROR_CODE(int_no) \
void __declspec(naked) trap_entry_##int_no()  \
{  \
	__asm hlt \
	__asm cli \
	/*__asm push -1  错误吗 */ \
	__asm push		int_no  /*中断向量*/ \
	__asm pushad \
	__asm push		ds \
	__asm push		es \
	__asm push		fs \
	__asm push		gs \
	__asm mov		eax,		esp \
	__asm push      eax \
	__asm call		TRAP::dispatch \
	__asm pop       eax \
	__asm pop		gs \
	__asm pop		fs \
	__asm pop		es \
	__asm pop		ds \
	__asm popad		\
	__asm add		esp, 8 /*跳过中断向量、错误码*/ \
	__asm sti \
	__asm iretd \
}

#define TRAP_NO_ERROR_CODE(int_no) \
void __declspec(naked) trap_entry_##int_no()  \
{  \
	__asm hlt \
	__asm cli \
	__asm push -1  /* 错误吗 */ \
	__asm push		int_no  /*中断向量*/ \
	__asm pushad \
	__asm push		ds \
	__asm push		es \
	__asm push		fs \
	__asm push		gs \
	__asm mov		eax,		esp \
	__asm push      eax \
    __asm call		TRAP::dispatch \
	__asm pop       eax \
	__asm pop		gs \
	__asm pop		fs \
	__asm pop		es \
	__asm pop		ds \
	__asm popad		\
	__asm add		esp, 8 /*跳过中断向量、错误码*/ \
	__asm sti \
	__asm iretd \
}

#pragma warning (disable:4100)

TRAP_NO_ERROR_CODE(0) //除法错误 
TRAP_NO_ERROR_CODE(1) //单步调试	INT 1
TRAP_NO_ERROR_CODE(2) //非屏蔽中断NMI
TRAP_NO_ERROR_CODE(3) //断点	INT 3
TRAP_NO_ERROR_CODE(4) //溢出中断	INTO
TRAP_NO_ERROR_CODE(5) //边界越界  BOUND
TRAP_NO_ERROR_CODE(6) //无效指令   UD2
TRAP_NO_ERROR_CODE(7) //! device not available
TRAP_WITH_ERROR_CODE(8) //! double fault
TRAP_NO_ERROR_CODE(9)	//协处理器跨段
TRAP_WITH_ERROR_CODE(10)//无效TSS
TRAP_WITH_ERROR_CODE(11)//段不存在
TRAP_WITH_ERROR_CODE(12)//栈故障
TRAP_WITH_ERROR_CODE(13)//常规保护
TRAP_WITH_ERROR_CODE(14)//页故障
TRAP_WITH_ERROR_CODE(15)//15(0x0F) 因特尔保留
TRAP_NO_ERROR_CODE(16)//浮点处理器错误
TRAP_WITH_ERROR_CODE(17)//对齐检查
TRAP_NO_ERROR_CODE(18)//机器检查
TRAP_NO_ERROR_CODE(19)//SIMD异常


#define EFLAGS_TF	0x00000100

#define SET_TRAP_ENTRY(irq_no) idt->set_trap_gate(irq_no,trap_entry_##irq_no)

#define TRAP_ENTRY(irq_no) trap_entry_##irq_no


void TRAP::dispatch(TRAP_CONTEXT* context)
{
	uint32 _CR2 = 0;
	uint32 irq_no = context->irq_no;
	if (irq_no >= MAX_TRAP_ENTRIES)
	{
		return;
	}
	if (TRAP::m_handlers[irq_no] != NULL)
	{
		TRAP::m_handlers[irq_no](context);
		return;
	}
	switch (irq_no)
	{
	case 0:TRAP::handler0(context);break;
	case 1:TRAP::handler1(context);break;
	case 2:TRAP::handler2(context);break;
	case 3:TRAP::handler3(context);break;
	case 4:TRAP::handler4(context);break;
	case 5:TRAP::handler5(context);break;
	case 6:TRAP::handler6(context);break;
	case 7:TRAP::handler7(context);break;
	case 8:TRAP::handler8(context);break;
	case 9:TRAP::handler9(context);break;
	case 10:TRAP::handler10(context);break;
	case 11:TRAP::handler11(context);break;
	case 12:TRAP::handler12(context);break;
	case 13:TRAP::handler13(context);break;
	case 14:TRAP::handler14(context);break;
	case 15:TRAP::handler15(context);break;
	case 16:TRAP::handler16(context);break;
	case 17:TRAP::handler17(context);break;
	case 18:TRAP::handler18(context);break;
	case 19:TRAP::handler19(context);break;
	}
}


//20-31(0x14--0x1F) 因特尔保留

TRAP_HANDLER TRAP::m_handlers[];

void TRAP::Init(IDT* idt)
{
	SET_TRAP_ENTRY(0);
	SET_TRAP_ENTRY(1);
	SET_TRAP_ENTRY(2);
	SET_TRAP_ENTRY(3);
	SET_TRAP_ENTRY(4);
	SET_TRAP_ENTRY(5);
	SET_TRAP_ENTRY(6);
	SET_TRAP_ENTRY(7);
	SET_TRAP_ENTRY(8);
	SET_TRAP_ENTRY(9);
	SET_TRAP_ENTRY(10);
	SET_TRAP_ENTRY(11);
	SET_TRAP_ENTRY(12);
	SET_TRAP_ENTRY(13);
	SET_TRAP_ENTRY(14);
	SET_TRAP_ENTRY(15);
	SET_TRAP_ENTRY(16);
	SET_TRAP_ENTRY(17);
	SET_TRAP_ENTRY(18);
	SET_TRAP_ENTRY(19);
	memset(m_handlers, 0, sizeof(m_handlers));
}

void TRAP::register_handler(int irq_no, TRAP_HANDLER handler)
{
	if (irq_no >= 0 && irq_no < MAX_TRAP_ENTRIES)
	{
		TRAP::m_handlers[irq_no] = handler;
	}
}

//除法错误 
void TRAP::handler0(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf,"int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);
}

//单步调试	INT 1
void TRAP::handler1(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);
}
//非屏蔽中断NMI
void TRAP::handler2(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//断点	INT 3
void TRAP::handler3(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//溢出中断	INTO
void TRAP::handler4(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//边界越界  BOUND
void TRAP::handler5(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//无效指令   UD2
void TRAP::handler6(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//! device not available
void TRAP::handler7(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//! double fault
void TRAP::handler8(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//协处理器跨段
void TRAP::handler9(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//无效TSS
void TRAP::handler10(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);
}
//段不存在
void TRAP::handler11(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//栈故障
void TRAP::handler12(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//常规保护
void TRAP::handler13(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//页故障
void TRAP::handler14(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	//sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	//DBG(buf);

		__asm cli
		uint32 errcode = context->err_code & 0x7;
		uint32 virt_addr = 0;
		__asm mov eax, cr2
		__asm mov virt_addr, eax
		switch (errcode)
		{
		case 0://系统级 读 不存在的页面
			sprintf(buf, "系统级 读 不存在的页面int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
			DBG(buf);
		case 1://系统级 读 违反页面保护权限
			sprintf(buf, "系统级 读 违反页面保护权限int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
			DBG(buf);
		case 2://系统级 写 不存在的页面
			sprintf(buf, "系统级 写 不存在的页面int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
			DBG(buf);
		case 3://系统级 写 违反页面保护权限
			sprintf(buf, "系统级 写 违反页面保护权限int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
			DBG(buf);
			break;
		case 4://用户级 读 不存在的页面
			sprintf(buf, "用户级 读 不存在的页面int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
			DBG(buf);
			break;
		case 6://用户级 写 不存在的页面
			sprintf(buf, "用户级 写 不存在的页面int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
			DBG(buf);
			break;
		case 5://用户级 读 违反页面保护权限
			sprintf(buf, "用户级 读 违反页面保护权限int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
			DBG(buf);
		case 7://用户级 写 违反页面保护权限
			sprintf(buf, "用户级 写 违反页面保护权限int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
			DBG(buf);
			break;
		}

		//printf("int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->int_no, context->err_code, virt_addr, context->cs, context->eip);
		//Pager::set_mem_attribute(virt_addr & 0xFFFFF000, PAGE_SIZE, PAGE_ATTR_PRESENT | PAGE_ATTR_WRITE | PAGE_ATTR_USER);
		//context->eflags |= EFLAGS_TF;
		//__asm hlt
}
//15(0x0F) 因特尔保留
void TRAP::handler15(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//浮点处理器错误
void TRAP::handler16(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//对齐检查
void TRAP::handler17(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//机器检查
void TRAP::handler18(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}
//SIMD异常
void TRAP::handler19(TRAP_CONTEXT* context)
{
	char buf[1024];
	uint32_t _CR2;
	__asm mov eax, CR2
	__asm mov _CR2, eax
	sprintf(buf, "int_no=%d errorcode=%08X CR2=%08X CS:EIP=%04X:%08X\n", context->irq_no, context->err_code, _CR2, context->cs, context->eip);
	DBG(buf);

}


