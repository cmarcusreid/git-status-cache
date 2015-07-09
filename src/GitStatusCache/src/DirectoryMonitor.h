#pragma once

#include <ReadDirectoryChanges.h>

class DirectoryMonitor : boost::noncopyable
{
public:
	enum FileAction
	{
		Unknown,
		Added,
		Removed,
		Modified,
		RenamedFrom,
		RenamedTo
	};

	using OnChangeCallback = std::function<void(const std::wstring&, FileAction)>;
	using OnEventsLostCallback = std::function<void(void)>;

private:
	HANDLE m_stopNotificationThread;
	std::thread m_notificationThread;

	OnChangeCallback m_onChangeCallback;
	OnEventsLostCallback m_onEventsLostCallback;

	CReadDirectoryChanges m_readDirectoryChanges;

public:
	DirectoryMonitor(const OnChangeCallback& onChangeCallback, const OnEventsLostCallback& onEventsLostCallback);
	~DirectoryMonitor();

	void AddDirectory(const std::wstring& directory);
};