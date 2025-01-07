#include "Cache.h"
#include "Platform.h"
#include "HTTP.h"
#include "ini.h"
#include <time.h>
#include <string.h>
#include <limits.h>

const int DefaultMaxAge = 3600;
const int MinimumCacheTime = 300;
const char* const ExpiresHeader = "Expires: ";
const char* const CacheControlHeader = "Cache-control: ";

const char* const NoStore = "no-store";
const char* const NoCache = "no-cache";
const char* const MaxAgeZero = "max-age=0";
const char* const MaxAge = "max-age=";

const char* const CacheInfoFile = "cache.inf";
static char cacheInfoPath[_MAX_PATH];

struct CacheEntry
{
	int id;
	char *url;
	long expiry;
	char *contentType;

	CacheEntry() : id(0), url(NULL), expiry(0), contentType(NULL) {}
	CacheEntry(int i, const char* u, long e, const char* c) : id(i), url(strdup(u)), expiry(e), contentType(strdup(c)) {}

	~CacheEntry()
	{
		free((void*)url);
		free((void*)contentType);
	}
};

int Cache::GetFreeId()
{
	for(int id = 1; id < INT_MAX; ++id)
	{
		bool found = false;
		for(size_t i = 0; i < cacheEntryCount; ++i)
		{
			if(entries[i]->id == id)
			{
				found = true;
				break;
			}
		}
		if(!found) return id;
	}
	return -1;
}

void Cache::ReadCache()
{
	loadEntry = NULL;
	snprintf(cacheInfoPath, _MAX_PATH, "%s\\%s\\%s", Platform::InstallPath(), Platform::config.cachePath, CacheInfoFile);
	ini_parse(cacheInfoPath, &CacheLoadHandler, this);
	if(loadEntry)
	{
		AddEntry(loadEntry->id, loadEntry->url, loadEntry->expiry, loadEntry->contentType);
		delete loadEntry;
	}
}

void Cache::WriteCache()
{
	snprintf(cacheInfoPath, _MAX_PATH, "%s\\%s\\%s", Platform::InstallPath(), Platform::config.cachePath, CacheInfoFile);
	FILE *f = fopen(cacheInfoPath, "w");
	for(size_t i = 0; i < cacheEntryCount; ++i)
	{
		fprintf(f, "[%i]\n", entries[i]->id);
		fprintf(f, "url = %s\n", entries[i]->url);
		fprintf(f, "expiry = %li\n", entries[i]->expiry);
		fprintf(f, "content-type = %s\n", entries[i]->contentType);
		fprintf(f, "\n");
	}
	fclose(f);
}

CacheEntry *Cache::AddEntry(int id, char *url, long expiry, char *contentType)
{
	if(id < 0) id = GetFreeId();
	else
	{
		for(size_t i = 0; i < cacheEntryCount; ++i)
		{
			if(entries[i]->id == id) return entries[i];
		}
	}

	++cacheEntryCount;
	entries = (CacheEntry**)realloc((void*)entries, cacheEntryCount * sizeof(CacheEntry*));
	entries[cacheEntryCount - 1] = new CacheEntry(id, url, expiry, contentType);
	return entries[cacheEntryCount - 1];
}

void Cache::RemoveEntry(int id)
{
	int index = -1;
	for(size_t i = 0; i < cacheEntryCount; ++i)
	{
		if(entries[i]->id == id) index = i;
	}
	if(index < 0) return; //Not found

	if(index < cacheEntryCount - 1)
	{
		memmove(&entries[index], &entries[index + 1], (cacheEntryCount - index - 1) * sizeof(CacheEntry*));
	}
	--cacheEntryCount;
	char cacheDataPath[_MAX_PATH];
	snprintf(cacheDataPath, _MAX_PATH, "%s\\%s\\%i.dat", Platform::InstallPath(), Platform::config.cachePath, id);
	remove(cacheDataPath);
}

int Cache::CacheLoadHandler(void* user, const char* section, const char* name, const char* value)
{
	Cache *that = (Cache*)user;
	int id = atoi(section);
	if(!that->loadEntry)
	{
		that->loadEntry = new CacheEntry();
		that->loadEntry->id = id;
	}
	else if(id != that->loadEntry->id)
	{
		that->AddEntry(that->loadEntry->id, that->loadEntry->url, that->loadEntry->expiry, that->loadEntry->contentType);
		delete that->loadEntry;
		that->loadEntry = new CacheEntry();
		that->loadEntry->id = id;
	}

	if(strcmp(name, "url") == 0)
	{
		that->loadEntry->url = strdup(value);
	}
	else if(strcmp(name, "expiry") == 0)
	{
		that->loadEntry->expiry = atol(value);
	}
	else if(strcmp(name, "content-type") == 0)
	{
		that->loadEntry->contentType = strdup(value);
	}

	return 1;
}

Cache::Cache() : entries((CacheEntry**)malloc(0)), cacheEntryCount(0) 
{
	Prune();
}

Cache& Cache::GetCache()
{
	static Cache cache;
	return cache;
}

FILE* Cache::Get(const char* url, time_t* expiry, char** contentType)
{
	Prune();
	CacheEntry* entry = NULL;
	for(size_t i = 0; i < cacheEntryCount; ++i)
	{
		if(strcmp(entries[i]->url, url) == 0)
		{
			entry = entries[i];
			break;
		}
	}
	if(!entry) return NULL;

	char cacheDataPath[_MAX_PATH];
	snprintf(cacheDataPath, _MAX_PATH, "%s\\%s\\%i.dat", Platform::InstallPath(), Platform::config.cachePath, entry->id);
	*expiry = entry->expiry;
	*contentType = entry->contentType;
	return fopen(cacheDataPath, "rb");
}

CacheWriter* Cache::Put(const char* url, time_t expiry, const char* contentType)
{
	int id = GetFreeId();
	CacheEntry* entry = new CacheEntry(id, url, expiry, contentType);
	return new CacheWriter(entry);
}

void Cache::Prune()
{
	bool run = true;
	while(run)
	{
		run = false;
		for(size_t i = 0; i < cacheEntryCount; ++i)
		{
			if(entries[i]->expiry < time(NULL))
			{
				RemoveEntry(entries[i]->id);
				run = true;
				break;
			}
		}
	}
	WriteCache();
}

static bool prefix(const char *pre, const char *str)
{
	return strnicmp(pre, str, strlen(pre)) == 0;
}

CacheInfo::CacheInfo() : cacheControlFound(false), cacheable(true), expiry(time(NULL) + DefaultMaxAge)
{}

static const char* const MonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL};

int GetMonth(const char* month)
{
	for(int i = 0; MonthNames[i]; ++i)
	{
		if(strcmp(MonthNames[i], month) == 0) return i;
	}
	return -1;
}

static tm ParseHTTPDateTime(const char *dateTime)
{
	int day, year, hour, minute, second;
	char month[4] = {0};
	int scanResult = sscanf(dateTime, "%*3s, %d %3s %d %d:%d:%d GMT", &day, &month, &year, &hour, &minute, &second);
	//Platform::Log("Scan result: %d", scanResult);
	tm ret;
	ret.tm_sec = second;
	ret.tm_min = minute;
	ret.tm_hour = hour;
	ret.tm_mday = day;
	ret.tm_mon = GetMonth(month);
	ret.tm_year = year - 1900;
	ret.tm_wday = -1;
	ret.tm_yday = -1;
	ret.tm_isdst = -1;
	return ret;
}

static void ProcessCacheControl(const char* header, CacheInfo &ci)
{
	if(strlen(header) < strlen(CacheControlHeader) + 1) return;
	if(strstr(header, NoCache) || strstr(header, NoStore) || strstr(header, MaxAgeZero))
	{
		ci.cacheable = false;
		ci.expiry = 0;
		return;
	}
	else
	{
		ci.cacheable = true;
	}
	const char* maxAgePos = strstr(header, MaxAge);
	if(maxAgePos)
	{
		const char* maxAgeValuePos = maxAgePos + strlen(MaxAge);
		if(maxAgeValuePos >= header + strlen(header)) return;
		long maxAge = atol(maxAgeValuePos);
		ci.expiry = time(NULL) + maxAge;
	}
	else
	{
		ci.expiry = time(NULL) + DefaultMaxAge;
	}
}

void CacheInfo::ParseHeader(const char* header)
{
	if(prefix(ExpiresHeader, header))
	{
		Platform::Log("Expires header: %s", header);
		tm expiresTm = ParseHTTPDateTime(header + strlen(ExpiresHeader));
		time_t expires = mktime(&expiresTm);
		
		char buffer[64];
		strftime(buffer, 64, "%F %T", &expiresTm);
		Platform::Log("  Interpreted as: %s (%d)", buffer, expires - time(NULL));

		if(cacheControlFound)
		{
			Platform::Log("  Cache-control takes precidence, expires NOT used.");
		}
		else
		{
			Platform::Log("  Cache-control not found (yet), setting values.");
			expiry = expires;
			if(expiry < time(NULL)) cacheable = false;
		}
	}
	else if(prefix(CacheControlHeader, header))
	{
		cacheControlFound = true;
		Platform::Log("Cache-control header: %s", header);
		ProcessCacheControl(header, *this);
		Platform::Log("  Cacheable? %s", (cacheable ? "Yes" : "No"));
		Platform::Log("  Expiry: %li", expiry - time(NULL));
	}
}

bool CacheInfo::ShouldCache()
{
	return cacheable && (expiry > time(NULL) + MinimumCacheTime);
}

CacheWriter::CacheWriter(CacheEntry *e) : entry(e)
{
	char cacheDataPath[_MAX_PATH];
	snprintf(cacheDataPath, _MAX_PATH, "%s\\%s\\%i.dat", Platform::InstallPath(), Platform::config.cachePath, entry->id);
	f = fopen(cacheDataPath, "wb");
}

void CacheWriter::Write(void* buffer, size_t size)
{
	fwrite(buffer, size, 1, f);
}

void CacheWriter::Abort()
{
	fclose(f);
	f = NULL;
	char cacheDataPath[_MAX_PATH];
	snprintf(cacheDataPath, _MAX_PATH, "%s\\%s\\%i.dat", Platform::InstallPath(), Platform::config.cachePath, entry->id);
	remove(cacheDataPath);
}

void CacheWriter::Finish()
{
	fclose(f);
	f = NULL;
	Cache& cache = Cache::GetCache();
	cache.Prune();
	cache.AddEntry(entry->id, entry->url, entry->expiry, entry->contentType);
	cache.WriteCache();
}

CacheWriter::~CacheWriter()
{
	if(f) Abort();
	delete entry;
}