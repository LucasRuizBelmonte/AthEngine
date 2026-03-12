#include "app/Application.h"
#include "app/CrashHandler.h"
#include "tools/SceneValidation.h"

#include <cstring>
#include <cstdio>
#include <exception>

namespace
{
	void PrintUsage()
	{
		std::printf("Usage:\n");
		std::printf("  athengine\n");
		std::printf("  athengine --validate-scene <path-to-scene.athscene>\n");
	}
}

int main(int argc, char **argv)
{
	if (argc > 1)
	{
		if (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0)
		{
			PrintUsage();
			return 0;
		}

		if (std::strcmp(argv[1], "--validate-scene") == 0)
		{
			if (argc < 3)
			{
				std::fprintf(stderr, "Missing scene path for --validate-scene.\n");
				PrintUsage();
				return 2;
			}

			return tools::RunSceneValidation(argv[2]);
		}

		std::fprintf(stderr, "Unknown command-line argument: %s\n", argv[1]);
		PrintUsage();
		return 2;
	}

	CrashHandler::Install();

	try
	{
		Application app;
		app.Run();
		return 0;
	}
	catch (const std::exception &e)
	{
		std::fprintf(stderr, "Application exited with exception: %s\n", e.what());
		return 1;
	}
	catch (...)
	{
		std::fprintf(stderr, "Application exited with unknown exception.\n");
		return 1;
	}
}
