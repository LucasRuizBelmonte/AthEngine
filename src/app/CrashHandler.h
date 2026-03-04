/**
 * @file CrashHandler.h
 * @brief Process-wide crash reporting hooks.
 */

#pragma once

namespace CrashHandler
{
	/**
	 * @brief Installs global crash handlers.
	 *
	 * Installs handlers for unhandled exceptions, std::terminate and common
	 * fatal C signals to print a stack trace before process exit.
	 */
	void Install();
}

