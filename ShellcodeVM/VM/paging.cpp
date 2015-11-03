#include "paging.h"

vm_pager::vm_pager()
{
}


vm_pager::~vm_pager()
{
}

uint32_t vm_pager::new_page_dir()
{
	//分配一物理页面
	uint32_t cr3 = PAGE_FRAME_DB::alloc_physical_page();
	//将其映射到一临时虚拟地址
	map_pages(cr3, PAGE_TMP_BASE, PAGE_SIZE, PT_PRESENT | PT_WRITABLE);
	//初始化清零
	memset((void*)PAGE_TMP_BASE, 0, PAGE_SIZE);
	//自映射
	((uint32_t*)PAGE_TMP_BASE)[PAGE_TABLE_BASE >> 22] = cr3 | PT_PRESENT | PT_WRITABLE;
	return cr3;
}

uint32_t vm_pager::new_page_table(uint32_t virtual_address)
{
	CHECK_PAGE_ALGINED(virtual_address);

	uint32_t  page_table_PA = PAGE_FRAME_DB::alloc_physical_page();
	uint32_t  val = page_table_PA | PT_PRESENT | PT_WRITABLE;
	if (USER_SPACE(virtual_address)) val |= PT_USER;
	SET_PDE(virtual_address, val);
	uint32_t page_table_VA = GET_PAGE_TABLE(virtual_address);
	memset((void*)page_table_VA, 0, PAGE_SIZE);
	return page_table_VA;
}

uint32_t vm_pager::map_pages(uint32_t physical_addr, uint32_t virtual_addr, int size, int protect)
{
	CHECK_PAGE_ALGINED(physical_addr);
	CHECK_PAGE_ALGINED(virtual_addr);
	CHECK_PAGE_ALGINED(size);

	uint32_t* page_dir = (uint32_t*)PAGE_DIR_BASE;
	if (USER_SPACE(virtual_addr)) protect |= PT_USER;

	uint32_t virtual_address = virtual_addr & 0xfffff000;
	uint32_t physical_address = physical_addr & 0xfffff000;
	uint32_t pages = ((virtual_addr - virtual_address) + size + PAGE_SIZE - 1) >> 12;

	for (uint32_t i = 0; i < pages; i++)
	{
		uint32_t va = virtual_address + i * PAGE_SIZE;
		uint32_t pa = physical_address + i * PAGE_SIZE;
		if (GET_PDE(va) == 0) new_page_table(va);
		SET_PTE(va, pa | protect);
	}
	return virtual_address;
}

void vm_pager::unmap_pages(uint32_t virtual_addr, int size)
{
	CHECK_PAGE_ALGINED(virtual_addr);
	CHECK_PAGE_ALGINED(size);

	uint32_t* page_dir = (uint32_t*)PAGE_DIR_BASE;

	uint32_t virtual_address = virtual_addr & 0xfffff000;
	uint32_t pages = ((virtual_addr - virtual_address) + size + PAGE_SIZE - 1) >> 12;

	for (uint32_t i = 0; i < pages; i++)
	{
		uint32_t va = virtual_address + i * PAGE_SIZE;
		uint32_t pd_index = PD_INDEX(va);
		uint32_t pt_index = PT_INDEX(va);
		uint32_t* page_table_VA = (uint32_t*)(PAGE_TABLE_BASE + (pd_index * PAGE_SIZE));
		if (page_dir[pd_index] != 0)
		{
			uint32_t page = page_table_VA[pt_index] >> 12;
			page_table_VA[pt_index] = 0;
			__asm { invlpg va } //无效TLB
			PAGE_FRAME_DB::free_physical_page(page);
		}
	}
}
