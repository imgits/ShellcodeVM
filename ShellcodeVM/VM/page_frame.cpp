#include "page_frame.h"
#include "paging.h"
#include <string.h>

page_frame_database::page_frame_database()
{
	m_mem_size = 0;
	m_page_frame_min = 0;
	m_page_frame_max = 0;
	m_next_free_page_frame = 0;
	m_page_frame_map = NULL;
}

page_frame_database::~page_frame_database()
{
	if (m_page_frame_map != NULL)
	{
		delete m_page_frame_map;
		m_page_frame_map = NULL;
	}
}

bool		page_frame_database::Init(pager*  pager, uint64_t mem_size)
{
	m_pager = pager;
	m_mem_size = mem_size;
	m_page_frame_min = SIZE_TO_PAGES(MB(4));
	m_page_frame_max = SIZE_TO_PAGES(mem_size);
	m_next_free_page_frame = 0;
	m_page_frame_map = new uint8_t(m_page_frame_max);
	memset(m_page_frame_map, 0, m_page_frame_max);
	uint64_t m_page_dir = pager->new_page_dir();
	return true;
}

uint64_t	page_frame_database::alloc_physical_page()
{
	for (int i = m_next_free_page_frame; i < m_page_frame_max; i++)
	{
		if (m_page_frame_map[i] == PAGE_FRAME_FREE)
		{
			m_next_free_page_frame = i + 1;
			return i*PAGE_SIZE;
		}
	}
	return 0;
}

uint64_t	page_frame_database::alloc_physical_pages(uint64_t pages)
{
	for (int i = m_next_free_page_frame; i < m_page_frame_max - pages; i++)
	{
		if (m_page_frame_map[i] == PAGE_FRAME_FREE)
		{
			int j = i;
			for (j++; j < i + pages && j < m_page_frame_max; j++)
			{
				if (m_page_frame_map[j] != PAGE_FRAME_FREE) break;
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

void	page_frame_database::free_physical_page(uint64_t page)
{
	m_page_frame_map[page] = PAGE_FRAME_FREE;
	if (page < m_next_free_page_frame)
	{
		m_next_free_page_frame = page;
	}
}

void	page_frame_database::free_physical_pages(uint64_t  start_page, uint64_t pages)
{
	for (uint64_t i = start_page; i < start_page + pages; i++)
	{
		m_page_frame_map[i] = PAGE_FRAME_FREE;
	}
	if (m_next_free_page_frame > start_page)
	{
		m_next_free_page_frame = start_page;
	}
}
