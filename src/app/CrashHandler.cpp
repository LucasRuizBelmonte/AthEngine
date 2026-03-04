/**
 * @file CrashHandler.cpp
 * @brief Crash reporting implementation.
 */

#include "CrashHandler.h"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <mutex>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#endif

namespace
{
	std::atomic<bool> g_installed{false};
	std::atomic<bool> g_handlingCrash{false};

#ifdef _WIN32
	std::once_flag g_symbolsInitOnce;
	std::mutex g_symbolMutex;

	void InitSymbols()
	{
		std::call_once(g_symbolsInitOnce, []()
					   {
			               HANDLE process = GetCurrentProcess();
			               SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
			               (void)SymInitialize(process, nullptr, TRUE); });
	}

	void PrintStackTrace(size_t framesToSkip)
	{
		InitSymbols();

		void *frames[128] = {};
		const USHORT captured = CaptureStackBackTrace(static_cast<DWORD>(framesToSkip + 1u), 128u, frames, nullptr);

		std::fprintf(stderr, "Stack trace (%u frames):\n", static_cast<unsigned>(captured));
		if (captured == 0)
		{
			std::fprintf(stderr, "  <no frames>\n");
			return;
		}

		std::lock_guard<std::mutex> lock(g_symbolMutex);
		HANDLE process = GetCurrentProcess();

		char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
		auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbolBuffer);
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;

		for (USHORT i = 0; i < captured; ++i)
		{
			const DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
			DWORD64 displacement = 0;

			IMAGEHLP_LINE64 lineInfo = {};
			lineInfo.SizeOfStruct = sizeof(lineInfo);
			DWORD lineDisplacement = 0;

			if (SymFromAddr(process, address, &displacement, symbol))
			{
				if (SymGetLineFromAddr64(process, address, &lineDisplacement, &lineInfo))
				{
					std::fprintf(stderr, "  #%u %s + 0x%llx (%s:%lu)\n",
								 static_cast<unsigned>(i),
								 symbol->Name,
								 static_cast<unsigned long long>(displacement),
								 lineInfo.FileName,
								 lineInfo.LineNumber);
				}
				else
				{
					std::fprintf(stderr, "  #%u %s + 0x%llx\n",
								 static_cast<unsigned>(i),
								 symbol->Name,
								 static_cast<unsigned long long>(displacement));
				}
			}
			else
			{
				std::fprintf(stderr, "  #%u 0x%llx\n",
							 static_cast<unsigned>(i),
							 static_cast<unsigned long long>(address));
			}
		}
	}
#else
	void PrintStackTrace(size_t)
	{
		std::fprintf(stderr, "Stack trace unsupported on this platform.\n");
	}
#endif

	[[noreturn]] void ExitAfterCrash()
	{
		std::fflush(stderr);
		std::_Exit(EXIT_FAILURE);
	}

	void PrintUnhandledCppExceptionMessage()
	{
		try
		{
			auto ex = std::current_exception();
			if (!ex)
				return;
			std::rethrow_exception(ex);
		}
		catch (const std::exception &e)
		{
			std::fprintf(stderr, "Unhandled C++ exception: %s\n", e.what());
		}
		catch (...)
		{
			std::fprintf(stderr, "Unhandled C++ exception: <non-std exception>\n");
		}
	}

	void ReportCrash(const char *reason)
	{
		const bool alreadyHandling = g_handlingCrash.exchange(true);
		if (alreadyHandling)
		{
			std::fprintf(stderr, "\n=== FATAL (recursive) ===\n");
			std::fprintf(stderr, "Reason: %s\n", reason ? reason : "<unknown>");
			ExitAfterCrash();
		}

		std::fprintf(stderr, "\n=== FATAL CRASH ===\n");
		std::fprintf(stderr, "Reason: %s\n", reason ? reason : "<unknown>");
		PrintStackTrace(2);
	}

#ifdef _WIN32
	LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS *exceptionInfo)
	{
		DWORD code = 0;
		const void *address = nullptr;
		if (exceptionInfo && exceptionInfo->ExceptionRecord)
		{
			code = exceptionInfo->ExceptionRecord->ExceptionCode;
			address = exceptionInfo->ExceptionRecord->ExceptionAddress;
		}

		char msg[256];
		std::snprintf(msg, sizeof(msg), "Unhandled SEH exception 0x%08lX at %p",
					  static_cast<unsigned long>(code), address);
		ReportCrash(msg);
		ExitAfterCrash();
	}
#endif

	void TerminateHandler()
	{
		ReportCrash("std::terminate invoked");
		PrintUnhandledCppExceptionMessage();
		ExitAfterCrash();
	}

	void SignalHandler(int sig)
	{
		switch (sig)
		{
		case SIGABRT:
			ReportCrash("Signal: SIGABRT");
			break;
		case SIGFPE:
			ReportCrash("Signal: SIGFPE");
			break;
		case SIGILL:
			ReportCrash("Signal: SIGILL");
			break;
		case SIGSEGV:
			ReportCrash("Signal: SIGSEGV");
			break;
		default:
			ReportCrash("Signal: unknown");
			break;
		}
		ExitAfterCrash();
	}
}

void CrashHandler::Install()
{
	if (g_installed.exchange(true))
		return;

#ifdef _WIN32
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#endif

	std::set_terminate(TerminateHandler);
	std::signal(SIGABRT, SignalHandler);
	std::signal(SIGFPE, SignalHandler);
	std::signal(SIGILL, SignalHandler);
	std::signal(SIGSEGV, SignalHandler);
}
