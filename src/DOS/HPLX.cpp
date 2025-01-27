#include "HPLX.h"
#include "../Platform.h"
#include <i86.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>

struct HPLXBitmap{
	int16_t Planes; /* on HP100/200, always 1 */
	int16_t Bits;   /* on HP100/200, always 1 */
	int16_t Width;
	int16_t Height;
};

bool Check5f()
{
	REGS regs;
	regs.w.ax = 0x1000;
	regs.w.bx = 0;
	regs.w.cx = 0x0a0b;
	regs.w.dx = 0;
	int86(0x5f, &regs, &regs);
	return regs.x.ax == 0x8;
}

const size_t smallBufferSize = (640/8) + sizeof(HPLXBitmap);
char smallImageBuffer[smallBufferSize];

void HPLXBlitLine(int x, int y, int w, void* imgdata, bool invert)
{
	// if(w <= 16)
	// {
	// 	Platform::Log("Width: %i Data: %x", w, (*(unsigned int*)imgdata));
	// 	REGS regs;
	// 	regs.w.cx = ((*(unsigned int*)imgdata) << (unsigned int)5);
	// 	Platform::Log("Width: %i Data: %x", w, regs.w.cx);
	// 	//if(invert) regs.w.cx = ~regs.w.cx;
	// 	regs.h.ah = 0x0B;
	// 	int86(0x5f, &regs, &regs);
	// 	regs.w.dx = y;
	// 	regs.w.cx = x + 16;//w;
	// 	regs.h.ah = 0x08;
	// 	int86(0x5f, &regs, &regs);
	// 	regs.w.dx = y;
	// 	regs.w.cx = x;
	// 	regs.h.ah = 0x06;
	// 	int86(0x5f, &regs, &regs);
	// 	return;
	// }

	REGS regs;
	SREGS sregs;

	int imgSize = (w + 7) / 8;
	int headerSize = sizeof(HPLXBitmap);
	char* data;
	if(imgSize + headerSize <= smallBufferSize)
	{
		data = smallImageBuffer;
	}
	else
	{
		data = (char*)malloc(imgSize + headerSize);
	}
	HPLXBitmap* bmp = new(data) HPLXBitmap();
	bmp->Planes = 1;
	bmp->Bits = 1;
	bmp->Width = w;
	bmp->Height = 1;
	memcpy(data + headerSize, imgdata, imgSize);

	segread(&sregs);
	regs.w.cx = x;
	regs.w.dx = y;
	regs.w.di = FP_OFF(data);
	sregs.es = FP_SEG(data);
	regs.h.ah = 0x0E;
	regs.h.al = invert ? 4 : 0;

	int86x(0x5f, &regs, &regs, &sregs);
	if(data != smallImageBuffer) free(data);
}

void GetLineToBuffer(int line, int width)
{
	HPLXBitmap* bmp = new(smallImageBuffer) HPLXBitmap();
	bmp->Planes = 1;
	bmp->Bits = 1;
	bmp->Width = width;
	bmp->Height = 1;

	REGPACK regs;
	memset(&regs, 0, sizeof(REGPACK));

	regs.w.dx = line;
	regs.w.cx = 0;
	regs.w.bp = line;
	regs.w.si = width;
	regs.w.di = FP_OFF(smallImageBuffer);
	regs.w.es = FP_SEG(smallImageBuffer);
	regs.h.ah = 0x0D;

	intr(0x5f, &regs);
}

void PutLineFromBuffer(int line, int width)
{
	REGS regs;
	SREGS sregs;
	segread(&sregs);

	regs.w.cx = 0;
	regs.w.dx = line;
	regs.w.di = FP_OFF(smallImageBuffer);
	sregs.es = FP_SEG(smallImageBuffer);
	regs.h.ah = 0x0E;
	regs.h.al = 0;

	int86x(0x5f, &regs, &regs, &sregs);
}

void HPLXScroll(int top, int bottom, int width, int amount)
{
	if(top + amount > bottom || top + amount < 0) return;
	
	void* buffer = malloc((((bottom - top) + 7) * width) + sizeof(HPLXBitmap));

	if(buffer)
	{
		REGPACK regs;
		memset(&regs, 0, sizeof(REGPACK));
		regs.w.dx = top + amount;
		regs.w.cx = 0;
		regs.w.bp = bottom;
		regs.w.si = width;
		regs.w.di = FP_OFF(buffer);
		regs.w.es = FP_SEG(buffer);
		regs.h.ah = 0x0D;
		intr(0x5f, &regs);

		memset(&regs, 0, sizeof(REGPACK));
		regs.w.cx = 0;
		regs.w.dx = top;
		regs.w.di = FP_OFF(buffer);
		regs.w.es = FP_SEG(buffer);
		regs.h.ah = 0x0E;
		regs.h.al = 0;
		intr(0x5f, &regs);

		free(buffer);
	}
	else
	{
		if (amount > 0)
		{
			for (int y = top; y < bottom; y++)
			{
				GetLineToBuffer(y + amount, width);
				PutLineFromBuffer(y, width);
			}
		}
		else if (amount < 0)
		{
			for (int y = bottom - 1; y >= top; y--)
			{
				GetLineToBuffer(y + amount, width);
				PutLineFromBuffer(y, width);
			}
		}
	}
}