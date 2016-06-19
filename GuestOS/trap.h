#pragma once
#include  "typedef.h"
#include  "idt.h"
#include  "gdt.h"


#pragma pack(push, 1)
struct TRAP_CONTEXT
{
	uint32	gs, fs, es, ds;
	uint32	edi, esi, ebp, tmp, ebx, edx, ecx, eax;
	uint32	irq_no, err_code;
	uint32	eip, cs, eflags, esp, ss;
};	//19*4=76 Bytes
#pragma pack(pop)

#define MAX_TRAP_ENTRIES		19

typedef void (*TRAP_HANDLER)(TRAP_CONTEXT* context);

class TRAP
{
private:
	static TRAP_HANDLER m_handlers[MAX_TRAP_ENTRIES];
public:
	static void Init(IDT* idt);
	static void register_handler(int irq_no, TRAP_HANDLER handler);
	static void dispatch(TRAP_CONTEXT* context);
	static void handler0(TRAP_CONTEXT* context);
	static void handler1(TRAP_CONTEXT* context);
	static void handler2(TRAP_CONTEXT* context);
	static void handler3(TRAP_CONTEXT* context);
	static void handler4(TRAP_CONTEXT* context);
	static void handler5(TRAP_CONTEXT* context);
	static void handler6(TRAP_CONTEXT* context);
	static void handler7(TRAP_CONTEXT* context);
	static void handler8(TRAP_CONTEXT* context);
	static void handler9(TRAP_CONTEXT* context);
	static void handler10(TRAP_CONTEXT* context);
	static void handler11(TRAP_CONTEXT* context);
	static void handler12(TRAP_CONTEXT* context);
	static void handler13(TRAP_CONTEXT* context);
	static void handler14(TRAP_CONTEXT* context);
	static void handler15(TRAP_CONTEXT* context);
	static void handler16(TRAP_CONTEXT* context);
	static void handler17(TRAP_CONTEXT* context);
	static void handler18(TRAP_CONTEXT* context);
	static void handler19(TRAP_CONTEXT* context);
};

