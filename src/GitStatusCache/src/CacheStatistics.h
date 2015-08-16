#pragma once

struct CacheStatistics
{
	uint64_t CacheHits = 0;
	uint64_t CacheMisses = 0;
	uint64_t CacheEffectivePrimeRequests = 0;
	uint64_t CacheTotalPrimeRequests = 0;
	uint64_t CacheEffectiveInvalidationRequests = 0;
	uint64_t CacheTotalInvalidationRequests = 0;
	uint64_t CacheInvalidateAllRequests = 0;
};