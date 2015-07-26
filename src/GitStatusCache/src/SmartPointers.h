#pragma once

using UniqueHandle = std::experimental::unique_resource_t<HANDLE, decltype(&::CloseHandle)>;

inline UniqueHandle MakeUniqueHandle(HANDLE handle)
{
	return std::experimental::unique_resource_checked(handle, INVALID_HANDLE_VALUE, &::CloseHandle);
}