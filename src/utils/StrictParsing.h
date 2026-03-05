#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace StrictParsing
{
	inline std::string TrimCopy(std::string text)
	{
		auto isNotSpace = [](unsigned char c)
		{ return !std::isspace(c); };

		auto begin = std::find_if(text.begin(), text.end(), isNotSpace);
		auto end = std::find_if(text.rbegin(), text.rend(), isNotSpace).base();
		if (begin >= end)
			return {};
		return std::string(begin, end);
	}

	inline bool ParseFloatList(const std::string &text, std::vector<float> &outValues, std::string &outError)
	{
		outValues.clear();

		std::string normalized = text;
		std::replace(normalized.begin(), normalized.end(), ',', ' ');
		std::istringstream ss(normalized);

		std::string token;
		while (ss >> token)
		{
			try
			{
				size_t parsed = 0u;
				const float value = std::stof(token, &parsed);
				if (parsed != token.size())
				{
					outError = "Invalid float token '" + token + "'.";
					return false;
				}
				outValues.push_back(value);
			}
			catch (...)
			{
				outError = "Invalid float token '" + token + "'.";
				return false;
			}
		}

		return true;
	}

	inline bool ParseIntList(const std::string &text, std::vector<int> &outValues, std::string &outError)
	{
		outValues.clear();

		std::string normalized = text;
		std::replace(normalized.begin(), normalized.end(), ',', ' ');
		std::istringstream ss(normalized);

		std::string token;
		while (ss >> token)
		{
			try
			{
				size_t parsed = 0u;
				const int value = std::stoi(token, &parsed);
				if (parsed != token.size())
				{
					outError = "Invalid int token '" + token + "'.";
					return false;
				}
				outValues.push_back(value);
			}
			catch (...)
			{
				outError = "Invalid int token '" + token + "'.";
				return false;
			}
		}

		return true;
	}

	template <typename T>
	inline bool RequireTokenCount(const std::vector<T> &values,
	                              size_t expected,
	                              const std::string &context,
	                              std::string &outError)
	{
		if (values.size() == expected)
			return true;

		std::ostringstream oss;
		oss << context << " expected " << expected << " token(s), found " << values.size() << ".";
		outError = oss.str();
		return false;
	}

	inline bool RequireKeys(const std::unordered_map<std::string, std::string> &map,
	                        const std::vector<std::string> &requiredKeys,
	                        const std::string &context,
	                        std::string &outError)
	{
		std::vector<std::string> missing;
		for (const std::string &key : requiredKeys)
		{
			const auto it = map.find(key);
			if (it == map.end() || it->second.empty())
				missing.push_back(key);
		}

		if (missing.empty())
			return true;

		std::ostringstream oss;
		oss << context << " missing required key(s): ";
		for (size_t i = 0; i < missing.size(); ++i)
		{
			if (i != 0u)
				oss << ", ";
			oss << "'" << missing[i] << "'";
		}
		outError = oss.str();
		return false;
	}

	inline bool ValidateFinite(float value,
	                           const std::string &fieldName,
	                           const std::string &context,
	                           std::string &outError)
	{
		if (std::isfinite(value))
			return true;

		outError = context + " has non-finite value for '" + fieldName + "'.";
		return false;
	}
}
