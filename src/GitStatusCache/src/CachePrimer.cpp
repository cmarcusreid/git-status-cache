#include "stdafx.h"
#include "CachePrimer.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>

CachePrimer::CachePrimer(const std::shared_ptr<Cache>& cache)
	: m_cache(cache)
	, m_stopPrimingThread(MakeUniqueHandle(INVALID_HANDLE_VALUE))
	, m_primingService()
	, m_primingTimer(m_primingService)
{
	auto stopPrimingThread = ::CreateEvent(
		nullptr /*lpEventAttributes*/,
		true    /*manualReset*/,
		false   /*bInitialState*/,
		nullptr /*lpName*/);
	if (stopPrimingThread == nullptr)
	{
		Log("CachePrimer.StartingPrimingThread.CreateEventFailed", Severity::Error)
			<< "Failed to create event to signal thread on exit.";
		throw std::runtime_error("CreateEvent failed unexpectedly.");
	}
	m_stopPrimingThread = MakeUniqueHandle(stopPrimingThread);

	Log("CachePrimer.StartingPrimingThread", Severity::Spam)
		<< "Attempting to start background thread for cache priming.";
	m_primingThread = std::thread(&CachePrimer::WaitForPrimingTimerExpiration, this);
}

CachePrimer::~CachePrimer()
{
	Log("CachePrimer.Shutdown.StoppingPrimingThread", Severity::Spam)
		<< R"(Shutting down cache priming thread. { "threadId": 0x)" << std::hex << m_primingThread.get_id() << " }";

	::SetEvent(m_stopPrimingThread);
	{
		WriteLock writeLock(m_primingMutex);
		m_primingTimer.cancel();
	}
	m_primingThread.join();
}

void CachePrimer::OnPrimingTimerExpiration(boost::system::error_code errorCode)
{
	if (errorCode.value() != 0)
		return;

	std::unordered_set<std::string> repositoriesToPrime;
	{
		WriteLock writeLock(m_primingMutex);
		m_repositoriesToPrime.swap(repositoriesToPrime);
	}

	if (!repositoriesToPrime.empty())
	{
		for (auto repositoryPath : repositoriesToPrime)
			m_cache->PrimeCacheEntry(repositoryPath);
	}

	this->SchedulePrimingInSixtySeconds();
}

void CachePrimer::WaitForPrimingTimerExpiration()
{
	Log("CachePrimer.WaitForPrimingTimerExpiration.Start", Severity::Verbose) << "Thread for cache priming started.";

	this->SchedulePrimingInSixtySeconds();
	do
	{
		m_primingService.run();
	} while (::WaitForSingleObject(m_stopPrimingThread, 5) != WAIT_OBJECT_0);

	Log("CachePrimer.WaitForPrimingTimerExpiration.Stop", Severity::Verbose) << "Thread for cache priming stopping.";
}

void CachePrimer::SchedulePrimingForRepositoryPathInFiveSeconds(const std::string& repositoryPath)
{
	WriteLock writeLock(m_primingMutex);
	m_repositoriesToPrime.insert(repositoryPath);
	if (m_primingTimer.expires_from_now(boost::posix_time::seconds(5)))
		m_primingTimer.async_wait([this](boost::system::error_code errorCode) { this->OnPrimingTimerExpiration(errorCode); });
}

void CachePrimer::SchedulePrimingInSixtySeconds()
{
	WriteLock writeLock(m_primingMutex);
	m_primingTimer.expires_from_now(boost::posix_time::seconds(60));
	m_primingTimer.async_wait([this](boost::system::error_code errorCode) { this->OnPrimingTimerExpiration(errorCode); });
}