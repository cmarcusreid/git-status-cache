#include "stdafx.h"
#include "DirectoryMonitor.h"

void DirectoryMonitor::WaitForNotifications()
{
	Log("DirectoryMonitor.WaitForNotifications.Start", Severity::Verbose) << "Thread for handling notifications started.";

	const HANDLE handles[] = { m_stopNotificationThread, m_readDirectoryChanges.GetWaitHandle() };

	bool shouldTerminate = false;
	while (!shouldTerminate)
	{
		auto alertable = true;
		DWORD waitResult = ::WaitForMultipleObjectsEx(_countof(handles), handles, false /*bWaitAll*/, INFINITE, true /*bAlertable*/);

		if (waitResult == WAIT_OBJECT_0)
		{
			shouldTerminate = true;
			Log("DirectoryMonitor.WaitForNotifications.Stop", Severity::Verbose) << "Thread for handling notifications stopping.";
		}
		else if (waitResult == WAIT_OBJECT_0 + 1)
		{
			if (m_readDirectoryChanges.CheckOverflow())
			{
				Log("DirectoryMonitor.Notification.Overflow", Severity::Warning)
					<< "Change notification queue overflowed. Notifications were lost.";
				m_onEventsLostCallback();
			}
			else
			{
				Token token;
				DWORD action;
				std::wstring path;
				m_readDirectoryChanges.Pop(token, action, path);

				auto fileAction = DirectoryMonitor::FileAction::Unknown;
				switch (action)
				{
				default:
					Log("DirectoryMonitor.Notification.Unknown", Severity::Warning)
						<< LR"(Unknown notification for file. { "token": )" << token << LR"(, "path": ")" << path << LR"(" })";
					break;
				case FILE_ACTION_ADDED:
					Log("DirectoryMonitor.Notification.Add", Severity::Spam)
						<< LR"(Added file. { "token": )" << token << LR"(, "path": ")" << path << LR"(" })";
					fileAction = DirectoryMonitor::FileAction::Added;
					break;
				case FILE_ACTION_REMOVED:
					Log("DirectoryMonitor.Notification.Remove", Severity::Spam)
						<< LR"(Removed file. { "token": )" << token << LR"(, "path": ")" << path << LR"(" })";
					fileAction = DirectoryMonitor::FileAction::Removed;
					break;
				case FILE_ACTION_MODIFIED:
					Log("DirectoryMonitor.Notification.Modified", Severity::Spam)
						<< LR"(Modified file. { "token": )" << token << LR"(, "path": ")" << path << LR"(" })";
					fileAction = DirectoryMonitor::FileAction::Modified;
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					Log("DirectoryMonitor.Notification.RenamedFrom", Severity::Spam)
						<< LR"(Renamed file. { "token": )" << token << LR"(, "oldPath": ")" << path << LR"(" })";
					fileAction = DirectoryMonitor::FileAction::RenamedFrom;
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					Log("DirectoryMonitor.Notification.RenamedTo", Severity::Spam)
						<< LR"(Renamed file. { "token": )" << token << LR"(, "newPath": ")" << path << LR"(" })";
					fileAction = DirectoryMonitor::FileAction::RenamedTo;
					break;
				}

				if (m_onChangeCallback != nullptr)
					m_onChangeCallback(token, boost::filesystem::path(path), fileAction);
			}
		}
	}
}

DirectoryMonitor::DirectoryMonitor(const OnChangeCallback& onChangeCallback, const OnEventsLostCallback& onEventsLostCallback) :
	m_onChangeCallback(onChangeCallback),
	m_onEventsLostCallback(onEventsLostCallback)
{
}

DirectoryMonitor::~DirectoryMonitor()
{
	Log("DirectoryMonitor.ShutDown", Severity::Verbose) << "Stopping directory monitor.";
	m_readDirectoryChanges.Terminate();

	if (m_stopNotificationThread != INVALID_HANDLE_VALUE)
	{
		Log("DirectoryMonitor.ShutDown.StoppingBackgroundThread", Severity::Spam)
			<< R"(Shutting down notification handling thread. { "threadId": 0x)" << std::hex << m_notificationThread.get_id() << " }";
		::SetEvent(m_stopNotificationThread);
		m_notificationThread.join();
		::CloseHandle(m_stopNotificationThread);
	}
}

DirectoryMonitor::Token DirectoryMonitor::AddDirectory(const std::wstring& directory)
{
	Token token;
	{
		boost::unique_lock<boost::shared_mutex> lock(m_directoriesMutex);
		auto iterator = m_directories.find(directory);
		if (iterator != m_directories.end())
			return iterator->second;

		static Token nextToken = 0;
		token = nextToken++;
		m_directories[directory] = token;
	}

	Log("DirectoryMonitor.AddDirectory", Severity::Info)
		<< LR"(Registering directory for change notifications. { "token": )" << token << LR"(, "path": ")" << directory << LR"(" })";

	auto notificationFlags =
		FILE_NOTIFY_CHANGE_LAST_WRITE
		| FILE_NOTIFY_CHANGE_CREATION
		| FILE_NOTIFY_CHANGE_FILE_NAME
		| FILE_NOTIFY_CHANGE_DIR_NAME
		| FILE_NOTIFY_CHANGE_SIZE;
	m_readDirectoryChanges.AddDirectory(directory.c_str(), token, true /*bWatchSubtree*/, notificationFlags);

	static std::once_flag flag;
	std::call_once(flag, [this]()
	{
		Log("DirectoryMonitor.StartingBackgroundThread", Severity::Spam)
			<< "Attempting to start background thread for handling notifications.";

		auto stopNotificationThread = ::CreateEvent(
			nullptr /*lpEventAttributes*/,
			true    /*manualReset*/,
			false   /*bInitialState*/,
			nullptr /*lpName*/);
		if (stopNotificationThread == nullptr)
		{
			Log("DirectoryMonitor.StartingBackgroundThread.CreateEventFailed", Severity::Error)
				<< "Failed to create event to signal thread on exit.";
			throw std::runtime_error("CreateEvent failed unexpectedly.");
		}
		m_stopNotificationThread = stopNotificationThread;

		m_notificationThread = std::thread(&DirectoryMonitor::WaitForNotifications, this);
	});

	return token;
}