#include "page_frame.h"
#include "paging.h"

page_frame_database::page_frame_database()
{
}


page_frame_database::~page_frame_database()
{
}

bool page_frame_database::Init(uint64_t host_va)
{
	m_host_va = host_va;

}
