//
// Copyright (C) 2021 James Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#include <stdio.h>
#include <windows.h>
#include "WinVid.h"
#include "../Image.h"
#include "../Interface.h"
#include "../DataPack.h"
#include "../Draw/Surf1bpp.h"
#include "../Draw/Surf8bpp.h"
#include "../Palettes.inc"

//#define SCREEN_WIDTH 800
//#define SCREEN_HEIGHT 600
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

extern HWND hWnd;

WindowsVideoDriver::WindowsVideoDriver()
{
	screenWidth = SCREEN_WIDTH;
	screenHeight = SCREEN_HEIGHT;
	foregroundColour = RGB(0, 0, 0);
	backgroundColour = RGB(255, 255, 255);
	verticalScale = ((screenWidth * 3.0f) / 4.0f) / screenHeight;
	//verticalScale = 1.0f;

	Assets.Load("Default.dat");
//	Assets.Load("EGA.dat");
	//Assets.Load("Lowres.dat");
//	Assets.Load("CGA.dat");
}

const RGBQUAD monoPalette[] =
{
	{ 0, 0, 0, 0 },
	{ 0xff, 0xff, 0xff, 0 }
};

const RGBQUAD cgaPalette[] =
{
	{ 0x00, 0x00, 0x00 }, // Entry 0 - Black
	{ 0xAA, 0x00, 0x00 }, // Entry 1 - Blue
	{ 0x00, 0xAA, 0x00 }, // Entry 2 - Green
	{ 0xAA, 0xAA, 0x00 }, // Entry 3 - Cyan
	{ 0x00, 0x00, 0xAA }, // Entry 4 - Red
	{ 0xAA, 0x00, 0xAA }, // Entry 5 - Magenta
	{ 0x00, 0x55, 0xAA }, // Entry 6 - Brown
	{ 0xAA, 0xAA, 0xAA }, // Entry 7 - Light Gray
	{ 0x55, 0x55, 0x55 }, // Entry 8 - Dark Gray
	{ 0xFF, 0x55, 0x55 }, // Entry 9 - Light Blue
	{ 0x55, 0xFF, 0x55 }, // Entry 10 - Light Green
	{ 0xFF, 0xFF, 0x55 }, // Entry 11 - Light Cyan
	{ 0x55, 0x55, 0xFF }, // Entry 12 - Light Red
	{ 0xFF, 0x55, 0xFF }, // Entry 13 - Light Magenta
	{ 0x55, 0xFF, 0xFF }, // Entry 14 - Yellow
	{ 0xFF, 0xFF, 0xFF }, // Entry 15 - White
};

void WindowsVideoDriver::Init()
{
	HDC hDC = GetDC(hWnd);
	HDC hDCMem = CreateCompatibleDC(hDC);

	bool useColour = true;
	int paletteSize = useColour ? 256 : 2;

	bitmapInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * paletteSize);
	ZeroMemory(bitmapInfo, sizeof(BITMAPINFO) + sizeof(RGBQUAD) * paletteSize);
	bitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo->bmiHeader.biWidth = screenWidth;
	bitmapInfo->bmiHeader.biHeight = screenHeight;
	bitmapInfo->bmiHeader.biPlanes = 1;
//	bitmapInfo->bmiHeader.biBitCount = 32;
	bitmapInfo->bmiHeader.biBitCount = useColour ? 8 : 1;
	bitmapInfo->bmiHeader.biCompression = BI_RGB;
	bitmapInfo->bmiHeader.biClrUsed = 0;

	if (useColour)
	{
		memcpy(bitmapInfo->bmiColors, cgaPalette, sizeof(RGBQUAD) * 16);
	}
	else
	{
		memcpy(bitmapInfo->bmiColors, monoPalette, sizeof(RGBQUAD) * 2);
	}

	screenBitmap = CreateDIBSection(hDCMem, bitmapInfo, DIB_RGB_COLORS, (VOID**)&lpBitmapBits, NULL, 0);

	if (useColour)
	{
		DrawSurface_8BPP* surface = new DrawSurface_8BPP(screenWidth, screenHeight);
		uint8_t* buffer = (uint8_t*)(lpBitmapBits);

		int pitch = screenWidth;

		for (int y = 0; y < screenHeight; y++)
		{
			int bufferY = (screenHeight - 1 - y);
			surface->lines[y] = &buffer[bufferY * pitch];
		}
		drawSurface = surface;

		for (int n = 0; n < screenWidth * screenHeight; n++)
		{
			buffer[n] = 0xf;
		}

		colourScheme.pageColour = 0xf;
		colourScheme.linkColour = 1;
		colourScheme.textColour = 0;
		colourScheme.buttonColour = 7;

		paletteLUT = cgaPaletteLUT;
	}
	else
	{
		DrawSurface_1BPP* surface = new DrawSurface_1BPP(screenWidth, screenHeight);
		uint8_t* buffer = (uint8_t*)(lpBitmapBits);
		
		int pitch = (screenWidth + 7) / 8;		// How many bytes needed for 1bpp
		if (pitch & 3)							// Round to nearest 32-bit 
		{
			pitch += 4 - (pitch & 3);
		}

		for (int y = 0; y < screenHeight; y++)
		{
			int bufferY = (screenHeight - 1 - y);
			surface->lines[y] = &buffer[bufferY * pitch];
		}
		drawSurface = surface;

		for (int n = 0; n < screenWidth * screenHeight / 8; n++)
		{
			buffer[n] = 0xff;
		}

		colourScheme.pageColour = 1;
		colourScheme.linkColour = 0;
		colourScheme.textColour = 0;
		colourScheme.buttonColour = 1;

		paletteLUT = nullptr;
	}
}

void WindowsVideoDriver::Shutdown()
{
}

void WindowsVideoDriver::ClearScreen()
{
	//FillRect(0, 0, screenWidth, TITLE_BAR_HEIGHT, foregroundColour);
	//FillRect(0, TITLE_BAR_HEIGHT, screenWidth, screenHeight - STATUS_BAR_HEIGHT - TITLE_BAR_HEIGHT, backgroundColour);
	//FillRect(0, screenHeight - STATUS_BAR_HEIGHT, screenWidth, STATUS_BAR_HEIGHT, foregroundColour);
}

void WindowsVideoDriver::SetPixel(int x, int y, uint32_t colour)
{
	if (x >= 0 && y >= 0 && x < screenWidth && y < screenHeight)
	{
		int outY = screenHeight - y - 1;
		uint8_t mask = 0x80 >> ((uint8_t)(x & 7));
		uint8_t* ptr = (uint8_t*)(lpBitmapBits);
		int index = (outY * (screenWidth / 8) + (x / 8));

		if (colour)
		{
			ptr[index] |= mask;
		}
		else
		{
			ptr[index] &= ~mask;
		}
	}
	/*
	if (x >= 0 && y >= 0 && x < screenWidth && y < screenHeight)
	{
		int line = ((screenHeight - y - 1) * verticalScale);
		for (int j = 0; j < verticalScale; j++)
		{
			lpBitmapBits[(line + j) * screenWidth + x] = colour;
		}
	}
	*/
}

void WindowsVideoDriver::InvertPixel(int x, int y, uint32_t colour)
{
	if (x >= 0 && y >= 0 && x < screenWidth && y < screenHeight)
	{
		int outY = screenHeight - y - 1;
		uint8_t mask = 0x80 >> ((uint8_t)(x & 7));
		uint8_t* ptr = (uint8_t*)(lpBitmapBits);
		int index = (outY * (screenWidth / 8) + (x / 8));

		ptr[index] ^= mask;
	}

	/*if (x >= 0 && y >= 0 && x < screenWidth && y < screenHeight)
	{
		int line = ((screenHeight - y - 1) * verticalScale);
		for (int j = 0; j < verticalScale; j++)
		{
			lpBitmapBits[(line + j) * screenWidth + x] ^= colour;
		}
	}*/
}

void WindowsVideoDriver::Paint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);
	HDC hdcMem = CreateCompatibleDC(hdc);
	HGDIOBJ oldBitmap = SelectObject(hdcMem, screenBitmap);
	BITMAP bitmap;

	GetObject(screenBitmap, sizeof(BITMAP), &bitmap);

	// Calculate the destination rectangle to stretch the bitmap to fit the window
	RECT destRect;
	GetClientRect(hwnd, &destRect);

	// Stretch the bitmap to fit the window size
	StretchBlt(hdc, 0, 0, destRect.right - destRect.left, destRect.bottom - destRect.top,
		hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);

	SelectObject(hdcMem, oldBitmap);
	DeleteDC(hdcMem);

	EndPaint(hwnd, &ps);
}

