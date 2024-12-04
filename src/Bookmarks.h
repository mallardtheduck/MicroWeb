#pragma once
#ifndef _BOOOKMARKS_H_
#define _BOOOKMARKS_H_

#include <stddef.h>
#include <memory>
#include "ini.h"

struct Bookmark
{
	size_t index;
	const char* title;
	const char* url;

	Bookmark(size_t i, const char* t, const char* u);
	~Bookmark();
};

class BookmarkList
{
	private:
		Bookmark **bookmarks;
		size_t count;
	public:
		const Bookmark& operator[](size_t i) const;
		size_t Count() const;

		BookmarkList(Bookmark** b, size_t count);
		~BookmarkList();
};

typedef BookmarkList* BookmarkListPtr;

BookmarkListPtr GetBookmarks();

void DeleteBookmark(size_t index);
void AddBookmark(const char* title, const char* url);

#endif