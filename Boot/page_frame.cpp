#include "page_frame.h"
#include "paging.h"
#include <string.h>

PAGE_FRAME_DB::PAGE_FRAME_DB()
{
	m_page_frame_min = 0;
	m_page_frame_max = 0;
	m_next_free_page_frame = 0;
	m_page_frame_database = (uint8_t*)PAGE_FRAME_BASE;
	m_database_usable = false;
}

PAGE_FRAME_DB::~PAGE_FRAME_DB()
{

}

bool	PAGE_FRAME_DB::Init(PAGER*  pager, uint32_t page_frame_min, uint32_t page_frame_max)
{
	m_pager = pager;
	m_page_frame_min = page_frame_min;
	m_page_frame_max = page_frame_max;
	m_next_free_page_frame = m_page_frame_min;
	m_database_usable = false;
	return true;
}

bool     PAGE_FRAME_DB::create_database()
{
	//为page_frame_db分配1M物理内存,映射到0xC00400000-0xC004FFFFF
	uint32_t  database_PA = alloc_physical_pages(SIZE_TO_PAGES(MB(1)));
	m_pager->map_pages(database_PA, PAGE_FRAME_BASE, MB(1));
	memset((void*)PAGE_FRAME_BASE, PAGE_FRAME_FREE, MB(1));

	m_next_free_page_frame = m_page_frame_min;
	m_database_usable = true;
	return true;
}

uint32_t PAGE_FRAME_DB::alloc_physical_page()
{
	if (!m_database_usable)
	{
		uint32_t addr = m_page_frame_min * PAGE_SIZE;
		m_page_frame_min++;
		m_next_free_page_frame = m_page_frame_min;
		return addr;
	}

	for (uint32_t i = m_next_free_page_frame; i < m_page_frame_max; i++)
	{
		if (m_page_frame_database[i] == PAGE_FRAME_FREE)
		{
			m_next_free_page_frame = i + 1;
			return i*PAGE_SIZE;
		}
	}
	return 0;
}

void   PAGE_FRAME_DB::free_physical_page(uint32_t page)
{
	m_page_frame_database[page] = PAGE_FRAME_FREE;
	if (page < m_next_free_page_frame)
	{
		m_next_free_page_frame = page;
	}
}

uint32_t PAGE_FRAME_DB::alloc_physical_pages(uint32_t pages)
{
	if (!m_database_usable)
	{
		uint32_t addr = m_page_frame_min * PAGE_SIZE;
		m_page_frame_min += pages;
		m_next_free_page_frame = m_page_frame_min;
		return addr;
	}

	for (uint32_t i = m_next_free_page_frame; i < m_page_frame_max - pages; i++)
	{
		if (m_page_frame_database[i] == PAGE_FRAME_FREE)
		{
			uint32_t j = i;
			for (j++; j < i + pages && j < m_page_frame_max; j++)
			{
				if (m_page_frame_database[j] != PAGE_FRAME_FREE) break;
			}
			if (j = i + pages)
			{
				m_next_free_page_frame = j;
				return i*PAGE_SIZE;
			}
		}
	}
	return 0;
}

void   PAGE_FRAME_DB::free_physical_pages(uint32_t  start_page, uint32_t pages)
{
	for (uint32_t i = start_page; i < start_page + pages; i++)
	{
		m_page_frame_database[i] = PAGE_FRAME_FREE;
	}
	if (m_next_free_page_frame > start_page)
	{
		m_next_free_page_frame = start_page;
	}
}

