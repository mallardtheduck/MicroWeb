#ifndef _HPLX_H_
#define _HPLX_H_

bool Check5f();
void HPLXBlitLine(int x, int y, int w, void* imgdata, bool invert);
void HPLXScroll(int top, int bottom, int w, int lines);

#endif