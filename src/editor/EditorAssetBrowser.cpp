#include "EditorModulesInternal.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>

namespace editorui::internal
{
	namespace
	{
		constexpr auto kAssetScanCacheTtl = std::chrono::milliseconds(1000);

		template <typename T>
		struct TimedScanCache
		{
			std::vector<T> entries;
			std::chrono::steady_clock::time_point lastRefresh{};
			bool valid = false;
		};

		template <typename T, typename Builder>
		const std::vector<T> &GetCachedEntries(TimedScanCache<T> &cache,
		                                       bool forceRefresh,
		                                       Builder &&builder)
		{
			const auto now = std::chrono::steady_clock::now();
			const bool cacheFresh = cache.valid && ((now - cache.lastRefresh) < kAssetScanCacheTtl);
			if (!forceRefresh && cacheFresh)
				return cache.entries;

			cache.entries = builder();
			cache.lastRefresh = now;
			cache.valid = true;
			return cache.entries;
		}
	}

	std::string TrimCopy(std::string text)
	{
		const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char c)
		                                    { return std::isspace(c) != 0; });
		const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c)
		                                  { return std::isspace(c) != 0; })
		                     .base();
		if (begin >= end)
			return {};
		return std::string(begin, end);
	}

	std::string ToLowerCopy(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
		               { return static_cast<char>(std::tolower(c)); });
		return text;
	}

	const std::vector<std::string> &CollectSceneAssetEntries(bool forceRefresh)
	{
		static TimedScanCache<std::string> cache;
		return GetCachedEntries(cache, forceRefresh, []()
		                        {
			                        std::vector<std::string> entries;
			                        std::error_code ec;

			                        const std::filesystem::path assetRoot = std::filesystem::path(ASSET_PATH).lexically_normal();
			                        if (!std::filesystem::exists(assetRoot, ec))
				                        return entries;

			                        std::filesystem::recursive_directory_iterator it(assetRoot, std::filesystem::directory_options::skip_permission_denied, ec);
			                        const std::filesystem::recursive_directory_iterator end;
			                        for (; it != end; it.increment(ec))
			                        {
				                        if (ec)
				                        {
					                        ec.clear();
					                        continue;
				                        }

				                        if (!it->is_regular_file(ec))
				                        {
					                        if (ec)
						                        ec.clear();
					                        continue;
				                        }

				                        const std::filesystem::path candidate = it->path();
				                        std::string ext = ToLowerCopy(candidate.extension().string());
				                        if (ext != ".athscene")
					                        continue;

				                        std::filesystem::path rel = std::filesystem::relative(candidate, assetRoot, ec);
				                        if (ec)
				                        {
					                        ec.clear();
					                        rel = candidate.lexically_normal();
				                        }

				                        entries.push_back(rel.generic_string());
			                        }

			                        std::sort(entries.begin(), entries.end());
			                        return entries; });
	}

	const std::vector<BasicShapeEntry> &CollectBasicShapeEntries(bool forceRefresh)
	{
		static TimedScanCache<BasicShapeEntry> cache;
		return GetCachedEntries(cache, forceRefresh, []()
		                        {
			                        std::vector<BasicShapeEntry> entries;
			                        std::error_code ec;

			                        const std::filesystem::path assetRoot = std::filesystem::path(ASSET_PATH).lexically_normal();
			                        const std::filesystem::path basicShapeRoot = assetRoot / "models" / "basicShapes";
			                        if (!std::filesystem::exists(basicShapeRoot, ec))
				                        return entries;

			                        std::filesystem::recursive_directory_iterator it(basicShapeRoot, std::filesystem::directory_options::skip_permission_denied, ec);
			                        const std::filesystem::recursive_directory_iterator end;
			                        for (; it != end; it.increment(ec))
			                        {
				                        if (ec)
				                        {
					                        ec.clear();
					                        continue;
				                        }

				                        if (!it->is_regular_file(ec))
				                        {
					                        if (ec)
						                        ec.clear();
					                        continue;
				                        }

				                        const std::filesystem::path candidate = it->path();
				                        const std::string ext = ToLowerCopy(candidate.extension().string());
				                        if (ext != ".fbx" && ext != ".obj" && ext != ".gltf" && ext != ".glb")
					                        continue;

				                        std::filesystem::path rel = std::filesystem::relative(candidate, assetRoot, ec);
				                        if (ec)
				                        {
					                        ec.clear();
					                        rel = candidate.lexically_normal();
				                        }

				                        entries.push_back(BasicShapeEntry{
					                        candidate.stem().string(),
					                        rel.generic_string()});
			                        }

			                        std::sort(entries.begin(), entries.end(), [](const BasicShapeEntry &a, const BasicShapeEntry &b)
			                                  { return ToLowerCopy(a.label) < ToLowerCopy(b.label); });
			                        return entries; });
	}

	const std::vector<std::string> &CollectTextureAssetEntries(bool forceRefresh)
	{
		static TimedScanCache<std::string> cache;
		return GetCachedEntries(cache, forceRefresh, []()
		                        {
			                        std::vector<std::string> entries;
			                        std::error_code ec;

			                        const std::filesystem::path assetRoot = std::filesystem::path(ASSET_PATH).lexically_normal();
			                        if (!std::filesystem::exists(assetRoot, ec))
				                        return entries;

			                        std::filesystem::recursive_directory_iterator it(assetRoot, std::filesystem::directory_options::skip_permission_denied, ec);
			                        const std::filesystem::recursive_directory_iterator end;
			                        for (; it != end; it.increment(ec))
			                        {
				                        if (ec)
				                        {
					                        ec.clear();
					                        continue;
				                        }

				                        if (!it->is_regular_file(ec))
				                        {
					                        if (ec)
						                        ec.clear();
					                        continue;
				                        }

				                        const std::filesystem::path candidate = it->path();
				                        const std::string ext = ToLowerCopy(candidate.extension().string());
				                        if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp" && ext != ".tga")
					                        continue;

				                        std::filesystem::path rel = std::filesystem::relative(candidate, assetRoot, ec);
				                        if (ec)
				                        {
					                        ec.clear();
					                        rel = candidate.lexically_normal();
				                        }

				                        entries.push_back(rel.generic_string());
			                        }

			                        std::sort(entries.begin(), entries.end());
			                        return entries; });
	}
}
