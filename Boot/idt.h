#pragma once
#include  <stdint.h>

#pragma pack(push, 1)
// 门描述符
typedef struct GATE_DESC
{
	uint16_t	offset_low;
	uint16_t	selector;
	uint8_t		dcount;
	uint8_t		attr;
	uint16_t	offset_high;
}GATE_DESC;

struct IDTR
{
	uint16_t	limit;
	void*		base;
};

#pragma pack(pop)

#define	MAX_IDT_NUM		256

#define	IDT_TSS_GATE		0x89	//可用 386 任务状态段类型值
#define	IDT_CALL_GATE	0x8C	//调用门类型值
#define	IDT_INTR_GATE	0x8E	//中断门类型值
#define	IDT_TRAP_GATE	0x8F	//陷阱门类型值			*/
//中断门和陷阱门的区别：http://blog.chinaunix.net/uid-9185047-id-445162.html
//通过中断门进入中断时，处理器自动清除IF标志位，返回时从栈中恢复原始状态
//通过陷阱门进入中断时，IF标志位保持不变


class IDT
{
private:
	GATE_DESC		m_idt[MAX_IDT_NUM];	//	256 gdt items
private:
	void set_idt_entry(int vector, int desc_type, void* handler);
public:
public:
	IDT(void);
	~IDT(void);
	void Init();
	void set_tss_gate(int vector, void* handler);
	void set_call_gate(int vector, void* handler);
	void set_intr_gate(int vector, void* handler);
	void set_trap_gate(int vector, void* handler);
};

extern IDT g_idt;