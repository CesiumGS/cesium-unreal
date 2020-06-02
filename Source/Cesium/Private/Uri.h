#pragma once

#include <string>

class Uri
{
public:
	static std::string resolve(const std::string& base, const std::string& relative, bool useBaseQuery = false);
	static std::string addQuery(const std::string& uri, const std::string& key, const std::string& value);
};