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

#ifndef _BIOSVID_H_
#define _BIOSVID_H_

#include "../Platform.h"

struct Image;

class BIOSVideoDriver : public VideoDriver
{
public:
	BIOSVideoDriver();

	virtual void Init(VideoModeInfo* inVideoModeInfo);
	virtual void Shutdown();

	virtual bool FastBlitLine(int x, int y, int w, void* data, bool invert);
	virtual bool FastScroll(int top, int bottom, int w, int lines);

private:
	int GetScreenMode();
	bool SetScreenMode(int screenMode);

	int startingScreenMode;
	bool hplxEnabled;
};

#endif
