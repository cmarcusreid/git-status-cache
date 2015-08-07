#pragma once

#include <boost/thread/shared_mutex.hpp>
#include <ReadDirectoryChanges.h>

/**
 * Monitors directories for changes and provides notifications by callback.
 */
class DirectoryMonitor : boost::noncopyable
{
public:
	/**
	 * Action that triggered change notification.
	 */
	enum FileAction
	{
		Unknown,
		Added,
		Removed,
		Modified,
		RenamedFrom,
		RenamedTo
	};

	/**
	 * Callback for change notifications. Provides path and change action.
	 */
	using OnChangeCallback = std::function<void(const boost::filesystem::path&, FileAction)>;

	/**
	 * Callback for events lost notification. Notifications may be lost if changes
	 * occur more rapidly than they are processed.
	 */
	using OnEventsLostCallback = std::function<void(void)>;

private:
	HANDLE m_stopNotificationThread = INVALID_HANDLE_VALUE;
	std::thread m_notificationThread;

	OnChangeCallback m_onChangeCallback;
	OnEventsLostCallback m_onEventsLostCallback;

	CReadDirectoryChanges m_readDirectoryChanges;

	std::unordered_set<std::wstring> m_directories;
	boost::shared_mutex m_directoriesMutex;

	void WaitForNotifications();

public:
	/**
	 * Constructor. Callbacks will always be invoked on the same thread.
	 * @param onChangeCallback Callback for change notification.
	 * @param onEventsLostCallback Callback for lost events notification.
	 */
	DirectoryMonitor(const OnChangeCallback& onChangeCallback, const OnEventsLostCallback& onEventsLostCallback);
	~DirectoryMonitor();

	/**
	 * Registers a directory for change notifications.
	 * This method is thread-safe.
	 */
	void AddDirectory(const std::wstring& directory);
};