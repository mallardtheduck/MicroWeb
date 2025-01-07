#pragma once
#ifndef _CACHE_H_
#define _CACHE_H_

#include <time.h>
#include <stdio.h>

struct CacheEntry;

struct CacheInfo
{
	bool cacheControlFound;

	bool cacheable;
	time_t expiry;

	CacheInfo();
	void ParseHeader(const char* header);

	bool ShouldCache();
};

class CacheWriter
{
	private:
		FILE* f;
		CacheEntry *entry;
	public:
		CacheWriter(CacheEntry *e);

		void Write(void* buffer, size_t size);
		void Abort();
		void Finish();

		~CacheWriter();
};

class Cache
{
	private:
		friend class CacheWriter;

		CacheEntry** entries;
		size_t cacheEntryCount;

		CacheEntry *loadEntry;
		
		int GetFreeId();
		void ReadCache();
		void WriteCache();
		CacheEntry* AddEntry(int id, char *url, long expiry, char *contentType);
		void RemoveEntry(int id);

		static int CacheLoadHandler(void* user, const char* section, const char* name, const char* value);
	public:
		Cache();

		static Cache& GetCache();

		FILE* Get(const char* url, time_t* expiry, char** contentType);
		CacheWriter* Put(const char* url, time_t expiry, const char* contentType);

		void Prune();
};

#endif