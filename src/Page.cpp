#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Platform.h"
#include "Page.h"

Page::Page()
{
	currentLineStartWidgetIndex = -1;
	currentWidgetIndex = -1;
	submittedWidgetIndex = 0;
	pageWidth = Platform::video->windowWidth;
	pageHeight = 0;
	numWidgets = 0;
	cursorX = 0;
	cursorY = 0;
	needLeadingWhiteSpace = false;
	pendingVerticalPadding = 0;

	styleStackSize = 1;
	styleStack[0] = WidgetStyle(FontStyle::Regular, 1, false);
}

WidgetStyle& Page::GetStyleStackTop()
{
	return styleStack[styleStackSize - 1];
}

void Page::PushStyle(const WidgetStyle& style)
{
	if (styleStackSize < MAX_PAGE_STYLE_STACK_SIZE)
	{
		styleStack[styleStackSize] = style;
		styleStackSize++;
		FinishCurrentWidget();
	}
}

void Page::PopStyle()
{
	if (styleStackSize > 1)
	{
		styleStackSize--;
	}
	FinishCurrentWidget();
}

Widget* Page::CreateWidget()
{
	if (currentWidgetIndex != -1)
	{
		FinishCurrentWidget();
	}

	if (numWidgets < MAX_PAGE_WIDGETS)
	{
		currentWidgetIndex = numWidgets;
		numWidgets++;

		Widget& current = widgets[currentWidgetIndex];
		current.style = GetStyleStackTop();

		if (needLeadingWhiteSpace)
		{
			if (cursorX > 0)
			{
				cursorX += Platform::video->GetGlyphWidth(' ', current.style.fontSize);
			}
			needLeadingWhiteSpace = false;
		}

		cursorY += pendingVerticalPadding;
		pendingVerticalPadding = 0;

		current.x = cursorX;
		current.y = cursorY;
		current.width = 0;
		current.height = Platform::video->GetLineHeight(current.style.fontSize);
		current.text = NULL;

		if (currentLineStartWidgetIndex == -1)
		{
			currentLineStartWidgetIndex = currentWidgetIndex;
		}

		return &current;
	}
	else
	{
		currentWidgetIndex = -1;
		return NULL;
	}
}

void Page::FinishCurrentWidget()
{
	if (currentWidgetIndex != -1)
	{
		Widget& current = widgets[currentWidgetIndex];
		cursorX += current.width;
		current.text = new char[textBufferSize + 1];

		if (current.text)
		{
			for (int n = 0; n < textBufferSize; n++)
			{
				current.text[n] = textBuffer[n];
			}
			current.text[textBufferSize] = '\0';
		}

		textBufferSize = 0;

		currentWidgetIndex = -1;
	}
}

void Page::FinishCurrentLine()
{
	FinishCurrentWidget();

	if (currentLineStartWidgetIndex != -1)
	{
		int lineWidth = 0;
		int lineHeight = 0;

		for (int n = currentLineStartWidgetIndex; n < numWidgets; n++)
		{
			if (widgets[n].height > lineHeight)
			{
				lineHeight = widgets[n].height;
			}
			if (widgets[n].x + widgets[n].width > lineWidth)
			{
				lineWidth = widgets[n].x + widgets[n].width;
			}
		}

		int centerAdjust = (Platform::video->windowWidth - lineWidth) >> 1;

		for (int n = currentLineStartWidgetIndex; n < numWidgets; n++)
		{
			widgets[n].y += lineHeight - widgets[n].height;
			if (widgets[n].style.center)
			{
				widgets[n].x += centerAdjust;
			}
		}

		cursorY += lineHeight;
		pageHeight = cursorY;

		// TODO render somewhere else?
		Platform::mouse->Hide();

		int scrollWidgetSize = pageHeight < Platform::video->windowHeight ? Platform::video->windowHeight : Platform::video->windowHeight * Platform::video->windowHeight / pageHeight;
		if (scrollWidgetSize < 8)
			scrollWidgetSize = 8;
		Platform::video->DrawScrollBar(0, scrollWidgetSize);

		for (int n = currentLineStartWidgetIndex; n < numWidgets; n++)
		{
			if (widgets[n].text)
			{
				Platform::video->DrawString(widgets[n].text, widgets[n].x, widgets[n].y + Platform::video->windowY, widgets[n].style.fontSize, widgets[n].style.fontStyle);
			}
		}
		Platform::mouse->Show();

		currentLineStartWidgetIndex = -1;
	}

	cursorX = 0;
}

void Page::AppendText(const char* text)
{
	if (currentWidgetIndex == -1)
	{
		CreateWidget();
	}

	if (currentWidgetIndex != -1)
	{
		Widget* current = &widgets[currentWidgetIndex];
		Font* font = Platform::video->GetFont(current->style.fontSize);

		int addedWidth = 0;
		int startIndex = 0;

		for (int index = 0; text[index] != '\0'; index++)
		{
			if (text[index] < 32 || text[index] > 128)
				continue;

			int glyphWidth = font->glyphWidth[text[index] - 32];
			if (current->style.fontStyle & FontStyle::Bold)
			{
				glyphWidth++;
			}

			if (current->x + current->width + addedWidth + glyphWidth < Platform::video->windowWidth)
			{
				addedWidth += glyphWidth;

				if (text[index] == ' ')
				{
					// Word boundary, so word fits, append now
					for (int n = startIndex; n <= index; n++)
					{
						if (textBufferSize < MAX_TEXT_BUFFER_SIZE)
						{
							textBuffer[textBufferSize++] = text[n];
						}
					}
					startIndex = index + 1;
					current->width += addedWidth;
					addedWidth = 0;
				}
			}
			else
			{
				if (current->width == 0)
				{
					if (current->x == 0)
					{
						// Case with lots of text without spaces, so just break in middle of the word
						for (int n = startIndex; n < index; n++)
						{
							if (textBufferSize < MAX_TEXT_BUFFER_SIZE)
							{
								textBuffer[textBufferSize++] = text[n];
							}
						}
						current->width += addedWidth;
						startIndex = index + 1;
						addedWidth = 0;

						FinishCurrentLine();

						current = CreateWidget();
					}
					else
					{
						// Case where widget is empty and needs to move to the next line

						// Move to new line
						FinishCurrentLine();
						currentLineStartWidgetIndex = currentWidgetIndex;
						current->x = cursorX;
						current->y = cursorY;
					}
				}
				else
				{
					// Case where we need to make a new widget on the next line
					FinishCurrentLine();
					current = CreateWidget();
				}

				if (!current)
				{
					// Couldn't make a new widget so probably out of memory
					return;
				}

				// Go back a character so it gets processed properly
				index--;
			}
		}

		// Add whatever is left
		if (addedWidth > 0)
		{
			for (int index = startIndex; text[index] != '\0'; index++)
			{
				if (textBufferSize < MAX_TEXT_BUFFER_SIZE)
				{
					textBuffer[textBufferSize++] = text[index];
				}
			}
			current->width += addedWidth;
		}
	}
}


void Page::Render()
{
	//FinishCurrentWidget();

	Platform::mouse->Hide();

	for (int n = 0; n < numWidgets; n++)
	{
		Widget& widget = widgets[n];
		if (widget.text)
		{
			Platform::video->DrawString(widget.text, widget.x, widget.y, widget.style.fontSize, widget.style.fontStyle);
		}
	}

	Platform::mouse->Show();
}

void Page::BreakLine(int padding)
{
	FinishCurrentLine();

	if (padding > pendingVerticalPadding)
	{
		pendingVerticalPadding = padding;
	}
}

void Page::SetTitle(const char* inTitle)
{
	//title = new char[strlen(inTitle) + 1];
	//strcpy(title, inTitle);

	Platform::mouse->Hide();
	Platform::video->DrawTitle(inTitle);
	Platform::mouse->Show();
}