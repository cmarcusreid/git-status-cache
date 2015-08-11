#pragma once

inline std::string ConvertToUtf8(const std::wstring& unicode)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(unicode);
}

inline std::wstring ConvertToUnicode(const std::string& utf8String)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(utf8String);
}