#include "paging.h"
#include <stdio.h>
#include <string.h>
#include "page_frame.h"

pager::pager()
{
}

pager::~pager()
{
}

bool  pager::Init(page_frame_database* page_frame_db, uint64_t host_mem, uint64_t page_table_base)
{
	m_host_mem = host_mem;
	m_page_frame_db = page_frame_db;
	m_page_table_base=page_table_base;
	return true;
}

uint64_t pager::new_page_dir()
{
	//分配一物理页面
	uint64_t cr3 = m_page_frame_db->alloc_physical_page();
	m_page_dir = GPA_TO_HVA(cr3);
	//初始化清零
	memset((void*)m_page_dir, 0, PAGE_SIZE);
	//自映射
	((uint32_t*)m_page_dir)[m_page_table_base >> 22] = cr3 | PT_PRESENT | PT_WRITABLE;
	return m_page_dir;
}

uint64_t pager::new_page_table(uint64_t virtual_address)
{
	CHECK_PAGE_ALGINED(virtual_address);

	uint64_t  page_table_PA = m_page_frame_db->alloc_physical_page();
	uint64_t  val = page_table_PA | PT_PRESENT | PT_WRITABLE;
	if (USER_SPACE(virtual_address)) val |= PT_USER;
	SET_PDE(virtual_address, val);
	uint64_t page_table_HVA = GPA_TO_HVA(page_table_PA);
	memset((void*)page_table_HVA, 0, PAGE_SIZE);
	return page_table_HVA;
}

uint64_t pager::map_pages(uint64_t physical_addr, uint64_t virtual_addr, uint64_t size, uint64_t protect)
{
	CHECK_PAGE_ALGINED(physical_addr);
	CHECK_PAGE_ALGINED(virtual_addr);
	CHECK_PAGE_ALGINED(size);

	uint64_t* page_dir = (uint64_t*)PAGE_DIR_BASE;
	if (USER_SPACE(virtual_addr)) protect |= PT_USER;

	uint64_t virtual_address = virtual_addr & 0xfffff000;
	uint64_t physical_address = physical_addr & 0xfffff000;
	uint64_t pages = ((virtual_addr - virtual_address) + size + PAGE_SIZE - 1) >> 12;

	for (uint64_t i = 0; i < pages; i++)
	{
		uint64_t va = virtual_address + i * PAGE_SIZE;
		uint64_t pa = physical_address + i * PAGE_SIZE;
		if (GET_PDE(va) == 0) new_page_table(va);
		SET_PTE(va, pa | protect);
	}
	return virtual_address;
}

void pager::unmap_pages(uint64_t virtual_addr, uint64_t size)
{
	CHECK_PAGE_ALGINED(virtual_addr);
	CHECK_PAGE_ALGINED(size);

	uint64_t virtual_address = virtual_addr & 0xfffff000;
	uint64_t pages = ((virtual_addr - virtual_address) + size + PAGE_SIZE - 1) >> 12;

	for (uint64_t i = 0; i < pages; i++)
	{
		uint64_t va = virtual_address + i * PAGE_SIZE;
		uint64_t page_table_PA = GET_PDE(va) & 0xfffff000;
		//uint64_t page_table_HVA = GPA_TO_HVA(page_table_PA);
		if (page_table_PA != 0)
		{
			uint64_t page = GET_PTE(va) >> 12;
			SET_PTE(va,0);
			m_page_frame_db->free_physical_page(page);
		}
	}
}
