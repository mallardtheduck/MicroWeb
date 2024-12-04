#include "Bookmarks.h"
#include <string.h>
#include <malloc.h>

const char* BookmarksFile = "bookmark.ini";

Bookmark::Bookmark(size_t i, const char* t, const char* u)
{
	index = i;
	title = strdup(t);
	url = strdup(u);
}

Bookmark::~Bookmark()
{
	free((void*)title);
	free((void*)url);
}

const Bookmark& BookmarkList::operator[](size_t i) const
{
	return *bookmarks[i];
}

size_t BookmarkList::Count() const
{
	return count;
}

BookmarkList::BookmarkList(Bookmark** b, size_t c) : count(c), bookmarks(b)
{
}

BookmarkList::~BookmarkList()
{
	for(size_t i = 0; i < count ; ++i)
	{
		delete bookmarks[i];
		bookmarks[i] = NULL;
	}
	free(bookmarks);
}

struct BookmarksLoadContext
{
	size_t count;
	Bookmark** array;
};


static int BookmarksLoadHandler(void* user, const char* section, const char* name, const char* value)
{
	BookmarksLoadContext* ctx = (BookmarksLoadContext*)user;

	if(strcmp(section, "bookmarks") == 0)
	{
		++ctx->count;
		ctx->array = (Bookmark**)realloc((void*)ctx->array, ctx->count * sizeof(Bookmark*));
		ctx->array[ctx->count - 1] = new Bookmark(ctx->count - 1, name, value);
	}

	return 1;
}

BookmarkListPtr GetBookmarks()
{
	BookmarksLoadContext ctx;
	ctx.count = 0;
	ctx.array = (Bookmark**)malloc(0);

	ini_parse(BookmarksFile, &BookmarksLoadHandler, &ctx);

	BookmarkListPtr retval(new BookmarkList(ctx.array, ctx.count));
	return retval;
}

static void WriteBookmarks(const BookmarkList& list, int skip, const char* addTitle, const char* addUrl)
{
	FILE *f = fopen(BookmarksFile, "w");
	fprintf(f, "[bookmarks]\n");
	size_t count = list.Count();
	for(size_t i = 0; i < count; ++i)
	{
		if(i == skip) continue;
		fprintf(f, "%s = %s\n", list[i].title, list[i].url);
	}
	if(addTitle && addUrl)
	{
		fprintf(f, "%s = %s\n", addTitle, addUrl);
	}
	fclose(f);
}

void DeleteBookmark(size_t index)
{
	BookmarkListPtr list = GetBookmarks();
	WriteBookmarks(*list, index, NULL, NULL);
}

void AddBookmark(const char* title, const char* url)
{
	BookmarkListPtr list = GetBookmarks();
	WriteBookmarks(*list, -1, title, url);
}
