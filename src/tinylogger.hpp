#pragma once

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

	void log(const char* message) {
		std::cout << message << std::endl;
	}

	LogLevel level;

private:

};

// Inline global logger instance (needs C++17 or later)
inline Logger logger(LogLevel::TRACE);