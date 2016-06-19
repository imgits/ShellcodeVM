#pragma once
#include <stdint.h>

#define  MAX_VIDEO_BUF_SIZE	0x10000
#define  MAX_SCREEN_WIDTH   255
#define  MAX_SCREEN_HEIGHT  255
#define  LINE_BUF_WIDTH		256

class VIDEO
{
private:
	uint32_t m_video_buf;
	uint32_t m_video_buf_size;
	uint32_t m_screen_width;
	uint32_t m_screen_height;
	uint32_t m_cursor_x;
	uint32_t m_cursor_y;
public:
	VIDEO();
	~VIDEO();
	 bool Init(uint32_t video_buf, uint32_t buf_size, uint32_t width, uint32_t height);
	 void putc(char ch);
	 void puts(char* str);
	 void clear();
	 void scroll(int lines);
	 void gotoxy(int x, int y);
	 int  getx();
	 int  gety();
};

extern VIDEO video;