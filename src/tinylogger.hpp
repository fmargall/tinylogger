#pragma once

#include <iostream>
#include <sstream>

enum class LogLevel {
	OFF      = 0,
	CRITICAL = 1,
	ERROR    = 2,
	WARNING  = 3,
	INFO     = 4,
	VERBOSE  = 5,
	DEBUG    = 6,
	TRACE    = 7
};

struct Logger {
public:
	Logger(LogLevel level) : level(level) {}

	template <typename... Args>
	void log(LogLevel logLevel, Args&&... args) const {
		std::ostringstream streamOutput;
		(streamOutput << ... << std::forward<Args>(args));
		std::cout << streamOutput.str() << std::endl;
	}

	LogLevel level;

private:

};

inline Logger logger(LogLevel::TRACE);