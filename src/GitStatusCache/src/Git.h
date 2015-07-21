#pragma once

/**
 * Performs git operations.
 */
class Git
{
private:
	static std::string ConvertToUtf8(const std::wstring& unicodeString);
	static std::wstring ConvertToUnicode(const std::string& utf8String);

public:
	Git();
	~Git();

	/**
	 * Searches for repository containing provided path.
	 */
	std::wstring DiscoverRepository(const std::wstring& path);
};