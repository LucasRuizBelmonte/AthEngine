#include "AthSceneIOInternal.h"

#include "../utils/StrictParsing.h"

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace AthSceneIO::internal
{
	SpritePivot SpritePivotFromStoredValue(int value)
	{
		switch (value)
		{
		case static_cast<int>(SpritePivot::TopLeft):
			return SpritePivot::TopLeft;
		case static_cast<int>(SpritePivot::Top):
			return SpritePivot::Top;
		case static_cast<int>(SpritePivot::TopRight):
			return SpritePivot::TopRight;
		case static_cast<int>(SpritePivot::Left):
			return SpritePivot::Left;
		case static_cast<int>(SpritePivot::Right):
			return SpritePivot::Right;
		case static_cast<int>(SpritePivot::BottomLeft):
			return SpritePivot::BottomLeft;
		case static_cast<int>(SpritePivot::Bottom):
			return SpritePivot::Bottom;
		case static_cast<int>(SpritePivot::BottomRight):
			return SpritePivot::BottomRight;
		case static_cast<int>(SpritePivot::Center):
		default:
			return SpritePivot::Center;
		}
	}

	bool IsValidSpritePivotValue(int value)
	{
		switch (value)
		{
		case static_cast<int>(SpritePivot::Center):
		case static_cast<int>(SpritePivot::TopLeft):
		case static_cast<int>(SpritePivot::Top):
		case static_cast<int>(SpritePivot::TopRight):
		case static_cast<int>(SpritePivot::Left):
		case static_cast<int>(SpritePivot::Right):
		case static_cast<int>(SpritePivot::BottomLeft):
		case static_cast<int>(SpritePivot::Bottom):
		case static_cast<int>(SpritePivot::BottomRight):
			return true;
		default:
			return false;
		}
	}

	UITextAlignment UITextAlignmentFromStoredValue(int value)
	{
		switch (value)
		{
		case static_cast<int>(UITextAlignment::Center):
			return UITextAlignment::Center;
		case static_cast<int>(UITextAlignment::Right):
			return UITextAlignment::Right;
		case static_cast<int>(UITextAlignment::Left):
		default:
			return UITextAlignment::Left;
		}
	}

	bool IsValidUITextAlignmentValue(int value)
	{
		return value == static_cast<int>(UITextAlignment::Left) ||
		       value == static_cast<int>(UITextAlignment::Center) ||
		       value == static_cast<int>(UITextAlignment::Right);
	}

	UIChildAlignment UIChildAlignmentFromStoredValue(int value)
	{
		switch (value)
		{
		case static_cast<int>(UIChildAlignment::Center):
			return UIChildAlignment::Center;
		case static_cast<int>(UIChildAlignment::End):
			return UIChildAlignment::End;
		case static_cast<int>(UIChildAlignment::Start):
		default:
			return UIChildAlignment::Start;
		}
	}

	bool IsValidUIChildAlignmentValue(int value)
	{
		return value == static_cast<int>(UIChildAlignment::Start) ||
		       value == static_cast<int>(UIChildAlignment::Center) ||
		       value == static_cast<int>(UIChildAlignment::End);
	}

	UIGridConstraint UIGridConstraintFromStoredValue(int value)
	{
		switch (value)
		{
		case static_cast<int>(UIGridConstraint::FixedRows):
			return UIGridConstraint::FixedRows;
		case static_cast<int>(UIGridConstraint::FixedColumns):
		default:
			return UIGridConstraint::FixedColumns;
		}
	}

	bool IsValidUIGridConstraintValue(int value)
	{
		return value == static_cast<int>(UIGridConstraint::FixedColumns) ||
		       value == static_cast<int>(UIGridConstraint::FixedRows);
	}

	UIFillDirection UIFillDirectionFromStoredValue(int value)
	{
		switch (value)
		{
		case static_cast<int>(UIFillDirection::RightToLeft):
			return UIFillDirection::RightToLeft;
		case static_cast<int>(UIFillDirection::LeftToRight):
		default:
			return UIFillDirection::LeftToRight;
		}
	}

	bool IsValidUIFillDirectionValue(int value)
	{
		return value == static_cast<int>(UIFillDirection::LeftToRight) ||
		       value == static_cast<int>(UIFillDirection::RightToLeft);
	}

	bool ReadExpectedKeyword(std::istream &in, const char *expected, std::string &outError)
	{
		std::string key;
		if (!(in >> key))
		{
			outError = "Unexpected end of file while reading scene file.";
			return false;
		}
		if (key != expected)
		{
			std::ostringstream oss;
			oss << "Invalid scene file. Expected '" << expected << "' but found '" << key << "'.";
			outError = oss.str();
			return false;
		}
		return true;
	}

	std::string ReadLinePayload(std::istream &in)
	{
		std::string payload;
		(void)std::getline(in, payload);
		if (!payload.empty() && payload.front() == ' ')
			payload.erase(payload.begin());
		return payload;
	}

	std::string BuildSchemaError(const std::string &componentName,
	                            const std::string &expectedSchema,
	                            const std::string &actualData)
	{
		std::ostringstream oss;
		oss << componentName << " schema mismatch. Expected " << expectedSchema
		    << ", actual '" << actualData << "'.";
		return oss.str();
	}

	bool ParseFloatPayload(const std::string &payload,
	                      const std::string &context,
	                      std::vector<float> &outValues,
	                      std::string &outError)
	{
		std::string parseError;
		if (!StrictParsing::ParseFloatList(payload, outValues, parseError))
		{
			outError = context + " invalid numeric token. " + parseError + " Actual '" + payload + "'.";
			return false;
		}
		return true;
	}

	void ClearRegistry(Registry &registry)
	{
		const std::vector<Entity> alive = registry.Alive();
		for (Entity entity : alive)
			registry.Destroy(entity);
	}
}
