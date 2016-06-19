//http://wiki.osdev.org/GDT_Tutorial
#pragma once
#include <stdint.h>
#define GDT_TO_SEL(gdt) ((gdt) << 3)

#define GDT_NULL			0
#define GDT_KERNEL_CODE		1
#define GDT_KERNEL_DATA		2
#define GDT_USER_CODE		3
#define GDT_USER_DATA		4
#define GDT_KERNEL_FS		5
#define GDT_USER_FS			6
#define GDT_SYS_TSS		7
#define GDT_TIB			8

#define SEL_KERNEL_CODE		0x08
#define SEL_KERNEL_DATA		0x10
#define SEL_USER_CODE		(0x18 | 0x03)
#define SEL_USER_DATA		(0x20 | 0x03)
#define SEL_KERNEL_FS		(0x28
#define SEL_USER_FS			(0x30 | 0x03)
#define SEL_SYS_TSS			(0x38
#define SEL_TIB				(0x40

#define MAX_GDT_NUM		32


/* 描述符类型值说明 */
#define	DA_16			0x0000	/* 16 位段				*/
#define	DA_32			0x4000	/* 32 位段				*/
#define	DA_LIMIT_4K		0x8000	/* 段界限粒度为 4K 字节			*/

#define	DA_DPL0			0x00	/* DPL = 0				*/
#define	DA_DPL1			0x20	/* DPL = 1				*/
#define	DA_DPL2			0x40	/* DPL = 2				*/
#define	DA_DPL3			0x60	/* DPL = 3				*/

/* 存储段描述符类型值说明 */
#define	DA_DR			0x90	/* 存在的只读数据段类型值		*/
#define	DA_DRW			0x92	/* 存在的可读写数据段属性值		*/
#define	DA_DRWA			0x93	/* 存在的已访问可读写数据段类型值	*/
#define	DA_C				0x98	/* 存在的只执行代码段属性值		*/
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值		*/
#define	DA_CCO			0x9C	/* 存在的只执行一致代码段属性值		*/
#define	DA_CCOR			0x9E	/* 存在的可执行可读一致代码段属性值	*/

/* 系统段描述符类型值说明 */
#define	DA_LDT			0x82	/* 局部描述符表段类型值			*/
#define	DA_TaskGate		0x85	/* 任务门类型值				*/
#define	DA_386TSS		0x89	/* 可用 386 任务状态段类型值		*/
#define	DA_386CGate		0x8C	/* 386 调用门类型值			*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值			*/
#define	DA_386TGate		0x8F	/* 386 陷阱门类型值			*/

/* 选择子类型值说明 */
/* 其中, SA_ : Selector Attribute */
#define	SA_RPL_MASK	0xFFFC
#define	SA_RPL0		0
#define	SA_RPL1		1
#define	SA_RPL2		2
#define	SA_RPL3		3

#define	SA_TI_MASK	0xFFFB
#define	SA_TIG		0
#define	SA_TIL		4

#define    KERNEL_FS_BASE				0xFFDFF000
#define    USER_FS_BASE				0x7EFDD000


#pragma pack(push, 1)
//段描述符
typedef struct SEGMENT_DESC
{
	uint16_t		    limit_low;
	uint16_t		    base_low;
	uint8_t			base_mid;
	uint8_t			attr1;
	uint8_t			limit_high_attr2;
	uint8_t			base_high;
}SEGMENT_DESC;

struct GDTR
{
	uint16_t		limit;
	void*		base;
};
#pragma pack(pop)

class GDT
{
private:
	SEGMENT_DESC m_gdt[MAX_GDT_NUM];
private:
	uint32_t GDT::find_free_gdt_entry();
	bool set_gdt_entry(uint32_t vector, uint32_t base, uint32_t limit, uint32_t attribute);
public:
	GDT();
	~GDT();
	void	Init();
	uint32_t  add_gdt_entry(uint32_t base, uint32_t limit, uint32_t attribute);
	uint32_t  add_ldt_entry(uint32_t base, uint32_t limit, uint32_t attribute);
	uint32_t  add_tss_entry(uint32_t base, uint32_t limit);
	uint32_t  add_call_gate(void* handler, int argc);

};

