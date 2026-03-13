#include "app/RuntimeApplication.h"
#include "app/CrashHandler.h"

#include <cstdio>
#include <cstring>
#include <exception>

namespace
{
	void PrintUsage()
	{
		std::printf("Usage:\n");
		std::printf("  athengine_runtime\n");
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

		std::fprintf(stderr, "Unknown command-line argument: %s\n", argv[1]);
		PrintUsage();
		return 2;
	}

	CrashHandler::Install();

	try
	{
		RuntimeApplication app;
		app.Run();
		return 0;
	}
	catch (const std::exception &e)
	{
		std::fprintf(stderr, "Runtime exited with exception: %s\n", e.what());
		return 1;
	}
	catch (...)
	{
		std::fprintf(stderr, "Runtime exited with unknown exception.\n");
		return 1;
	}
}
