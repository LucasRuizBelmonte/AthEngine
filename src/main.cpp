#include "app/Application.h"
#include "app/CrashHandler.h"
#include <fmod.hpp>
#include <cstdio>
#include <exception>

int main()
{
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
