#include "AthSceneIOInternal.h"

#include <filesystem>

namespace AthSceneIO
{
	std::string ScenePathResolver::ToRelativePathForSave(const std::string &rawPath)
	{
		if (rawPath.empty())
			return {};

		std::filesystem::path path(rawPath);
		path = path.lexically_normal();

		if (path.is_absolute())
		{
			const std::filesystem::path projectRoot = std::filesystem::path(ASSET_PATH).lexically_normal().parent_path();
			std::error_code ec;
			std::filesystem::path rel = std::filesystem::relative(path, projectRoot, ec);
			if (!ec && !rel.empty())
			{
				path = rel.lexically_normal();
			}
			else
			{
				ec.clear();
				const std::filesystem::path cwd = std::filesystem::current_path(ec);
				if (!ec)
				{
					rel = std::filesystem::relative(path, cwd, ec);
					if (!ec && !rel.empty())
						path = rel.lexically_normal();
				}
			}
		}

		return path.generic_string();
	}
}
