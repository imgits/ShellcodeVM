#pragma once
#include <stdint.h>
class page_frame_database
{
private:
	uint64_t m_host_va;
public:
	page_frame_database();
	~page_frame_database();
	bool Init(uint64_t host_va);
};

