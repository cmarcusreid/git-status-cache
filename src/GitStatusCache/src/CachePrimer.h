#pragma once
#include "Cache.h"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>

/**
* Actively updates invalidated cache entries to reduce cache misses on client requests.
* This class is thread-safe.
*/
class CachePrimer : boost::noncopyable
{
private:
	using ReadLock = boost::shared_lock<boost::shared_mutex>;
	using WriteLock = boost::unique_lock<boost::shared_mutex>;
	using UpgradableLock = boost::upgrade_lock<boost::shared_mutex>;
	using UpgradedLock = boost::upgrade_to_unique_lock<boost::shared_mutex>;

	std::shared_ptr<Cache> m_cache;

	UniqueHandle m_stopPrimingThread;
	std::thread m_primingThread;
	std::unordered_set<std::string> m_repositoriesToPrime;
	boost::asio::io_service m_primingService;
	boost::asio::deadline_timer m_primingTimer;
	boost::shared_mutex m_primingMutex;

	/**
	* Primes cache by computing status for scheduled repositories.
	*/
	void OnPrimingTimerExpiration(boost::system::error_code errorCode);

	/**
	* Reserves thread for priming operations until cache shuts down.
	*/
	void WaitForPrimingTimerExpiration();

public:
	CachePrimer(const std::shared_ptr<Cache>& cache);
	~CachePrimer();

	/**
	* Cancels any currently scheduled priming and reschedules for five seconds in the future.
	* Called repeatedly on file changes to refresh status for repositories five seconds after
	* a wave file change events (ex. a build) subsides.
	*/
	void SchedulePrimingForRepositoryPathInFiveSeconds(const std::string& repositoryPath);

	/**
	* Schedules cache priming for sixty seconds in the future.
	*/
	void SchedulePrimingInSixtySeconds();
};