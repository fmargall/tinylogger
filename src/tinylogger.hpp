#pragma once


/*
 * ============================================================================
 *                           TinyLogger Header File
 * ============================================================================
 * 
 * @author François Margall
 * @date   31-07-2025
 * 
 * This single-file header only library provides a simple and efficient logging
 * module for C++ programs.
 */


/* @brief Controls function inlining behavior for TinyLogger.
 * 
 * TinyLogger defines a macro IS_TINYLOGGER_INLINED (that can be set to 0 or 1)
 * that controls whether the logging functions are inlined or not. Be careful :
 * it is strongly recommended to read the following documentation before trying
 * any modification.
 * 
 * Supported compilers:
 *  - MSVC     : ('__forceinline')
 *  - GCC/Clang: ('inline __attribute__((always_inline))')
 *  - Others   : ('inline') Default value, which may not enforce inlining.
 * 
 * ==========    When should you set IS_TINYLOGGER_INLINED to 1 ?    ==========
 * 
 * This will strongly depend of the use case of the logger in your own program.
 * Before setting this value to 1, you should always profile your code, chosing
 * a representative use case, and check if the inlining is really needed.
 * 
 */
#ifndef    IS_TINYLOGGER_INLINED
#   define IS_TINYLOGGER_INLINED 0
#endif

#if (!IS_TINYLOGGER_INLINED)
    // Inlining is not forced, and will be selected by the compiler.
#   define INLINING_TINYLOGGER inline
#else
#   if   defined(_MSC_VER)
        // Use __forceinline for MSVC to enforce the inlining for all functions
#       define INLINING_TINYLOGGER __forceinline
#   elif defined(__GNUC__) || defined(__clang__)
        // Use inline __attribute__((always_inline)) for GCC / Clang compilers.
#       define INLINING_TINYLOGGER inline __attribute__((always_inline))
#   else
        // Inlining is asked for non-supported compilers, may not be enforced.
#       define INLINING_TINYLOGGER inline
#   endif
#endif


#ifndef    LOG_FUNCTION_NAME
#   define LOG_FUNCTION_NAME 1
#endif

#ifndef    LOG_FILE_NAME
#   define LOG_FILE_NAME 0
#endif

#ifndef    LOG_LINE_NUMBER
#   define LOG_LINE_NUMBER 0
#endif


#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <sstream>
#include <unordered_map>


enum class LogLevel {
    OFF      = 0,
    CRITICAL = 1,
    LERROR   = 2, // Avoiding potential conflict with <windows.h>
    WARNING  = 3,
    INFO     = 4,
    VERBOSE  = 5,
    DEBUG    = 6,
    TRACE    = 7
};

struct Logger {
public:
    /*
	 * @brief Constructs a Logger instance, with the specified log level.
     * 
	 * @param logLevel The initial log level for the logger. See LogLevel
	 *                 enum class for available log levels.
     * 
	 * @note /!\ Caution: This constructor will be called only once, when
     *           all the header is included in the code, thanks to inline
     *           Logger instance, declared at the end of this struct. 
	 *           The user, once that the header is included does not have
                 to declare another instance, and can directly use macros
     */
    Logger(LogLevel logLevel) : logLevel_(logLevel) {
		// Initialize the current time and last log time
        lastLogTime_ = std::chrono::system_clock::now();

        currentTimeInstanced_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        #ifdef _WIN32 // ctime_s is used for Windows compatibility
            ctime_s(currentTimeChar_, sizeof(currentTimeChar_), &currentTimeInstanced_); currentTimeChar_[24] = '\0';
        #else         // ctime_r is used for Linux compatibility
            ctime_r(&currentTimeInstanced_, currentTimeChar_); currentTimeChar_[24] = '\0';
        #endif        // Both are thread-safe while using ctime only is not
    }

    /*
	 * @brief Concatenates arguments, and logs at the specified level.
     * 
	 * This function will be the main entry point for some macros when
	 * maximum log level is defined at compilation time. For the other
	 * case, the macros will call the specific log functions directly.
     *
	 * @tparam Args (variadic): Any arguments, of any streamable type.
     * 
	 * @param logLevel Log level at which the message should be logged
	 * @param args    (variadic): Any arguments of any streamable type
     * 
     * @note It is strongly recommended to use the macros, rather than
	 *       this function directly. The macros will optimise the code
	 *       performance and may reduce the binary size.
     */
    template <typename... Args>
    INLINING_TINYLOGGER void log(LogLevel logLevel, Args&&... args) const {
        if (logLevel <= logLevel_) {
            switch (logLevel) {
                case LogLevel::TRACE:
				    logTRACE(args...);    break;
                case LogLevel::DEBUG:
                    logDEBUG(args...);    break;
                case LogLevel::VERBOSE:
                    logVERBOSE(args...);  break;
                case LogLevel::INFO:
                    logINFO(args...);     break;
                case LogLevel::WARNING:
                    logWARNING(args...);  break;
                case LogLevel::LERROR:
                    logERROR(args...);    break;
                case LogLevel::CRITICAL:
                    logCRITICAL(args...); break;
                default:
                    break;
            }
        }
    }

    template <typename... Args>
    INLINING_TINYLOGGER void logTRACE(Args&&... args) const {
        // Locks mutex during log for thread-safety
        std::lock_guard<std::mutex> guard(logMutex_);
        std::clog << "[TRACE]    " << formatMessage(std::forward<Args>(args)...) << std::endl;
    }

    template <typename... Args>
    INLINING_TINYLOGGER void logDEBUG(Args&&... args) const {
        // Locks mutex during log for thread-safety
        std::lock_guard<std::mutex> guard(logMutex_);
        std::clog << "[DEBUG]    " << formatMessage(std::forward<Args>(args)...) << std::endl;
    }

    template <typename... Args>
    INLINING_TINYLOGGER void logVERBOSE(Args&&... args) const {
        // Locks mutex during log for thread-safety
        std::lock_guard<std::mutex> guard(logMutex_);
        std::clog << "[VERBOSE]  " << formatMessage(std::forward<Args>(args)...) << std::endl;
    }

    template <typename... Args>
    INLINING_TINYLOGGER void logINFO(Args&&... args) const {
        // Locks mutex during log for thread-safety
        std::lock_guard<std::mutex> guard(logMutex_);
        std::cout <<"[INFO]     " << formatMessage(std::forward<Args>(args)...) << std::endl;
    }

    template <typename... Args>
    INLINING_TINYLOGGER void logWARNING(Args&&... args) const {
        // Locks mutex during log for thread-safety
        std::lock_guard<std::mutex> guard(logMutex_);
        std::clog << "[WARNING]  " << formatMessage(std::forward<Args>(args)...) << std::endl;
    }

    template <typename... Args>
    INLINING_TINYLOGGER void logERROR(Args&&... args) const {
        // Locks mutex during log for thread-safety
        std::lock_guard<std::mutex> guard(logMutex_);
        std::cerr << "[ERROR]    " << formatMessage(std::forward<Args>(args)...) << std::endl;
    }

    template <typename... Args>
    [[noreturn]] INLINING_TINYLOGGER void logCRITICAL(Args&&... args) const {
        // Locks mutex during log for thread-safety
        std::lock_guard<std::mutex> guard(logMutex_);
		std::cerr << "[CRITICAL] " << formatMessage(std::forward<Args>(args)...) << std::endl;
        exit(EXIT_FAILURE);
    }

    void setLogLevel(LogLevel logLevel) {
        // Locks mutex when logLevel update for safety
		std::lock_guard<std::mutex> guard(logMutex_);

        // Checks if the new log level is valid
        #ifdef         MAX_LOG_LEVEL_AT_COMPILATION
        if (logLevel > MAX_LOG_LEVEL_AT_COMPILATION) {
			logERROR("Invalid log level: ", static_cast<int>(logLevel), ". "
                     "Maximum allowed is: ", MAX_LOG_LEVEL_AT_COMPILATION);
            return;
        }
        #endif

        logLevel_ = logLevel;
    }

    void displayProgressBar(const size_t& currentIteration, const size_t& numberIterations) const {
        // Locks the mutex for thread-safety display
        std::lock_guard<std::mutex> guard(logMutex_);

		// Progress bar width parameter (in characters)
        static const size_t progressBarWidth = 50;

		double oldProgressPercentage = (static_cast<double>(currentIteration_.load()) / (numberIterations_.load() - 1)) * 100.0;
		double newProgressPercentage = (static_cast<double>(currentIteration)         / (numberIterations - 1))         * 100.0;

        // Progress bar is refreshed every new percentage value or for a new initialisation
        bool isLastIteration = (currentIteration == numberIterations - 1);
        if ((newProgressPercentage - oldProgressPercentage < 1) &&
            (numberIterations == numberIterations_.load())      &&
            (!isLastIteration))
            return;

        // Updating internal logging parameters
		currentIteration_.store(currentIteration);
        numberIterations_.store(numberIterations);
        
        size_t filledWidth = static_cast<size_t>(progressBarWidth * (newProgressPercentage / 100.0));
        std::string progressBar = "           [" 
                                  + std::string(filledWidth, '=') 
                                  + std::string(progressBarWidth - filledWidth, ' ')
                                  + "] ";

        // Displays the progress bar with percentage
        std::cout << progressBar << std::fixed << std::setprecision(0) << static_cast<int>(newProgressPercentage) << "%\r";
		std::cout.flush();
    }

    void addFlag(const std::string& flagName) const {
		// Locks mutex for thread-safety flag addition
        std::lock_guard<std::mutex> guard(flagMutex_);

        // Flag may already exist, and will be overwritten
        if (flagTimes_.find(flagName) != flagTimes_.end())
			logWARNING("Flag '", flagName, "' already exists and will be overwritten.");

        // Saving the current time for the flag in the flags map
        flagTimes_[flagName] = std::chrono::system_clock::now();
    }

    void releaseFlag(const std::string& flagName) const {
        // Locks mutex for thread-safety flag removal
        std::lock_guard<std::mutex> guard(flagMutex_);

        auto iterator  = flagTimes_.find(flagName);
        if  (iterator == flagTimes_.end())
			logERROR("Flag '", flagName, "' could not be found in memory.");
        else {
            // Avoid using directly currentTime and elapsedTime for thread-safety
            std::chrono::system_clock::time_point currentTimeFlag;
            std::chrono::milliseconds             elapsedTimeFlag;

            currentTimeFlag = std::chrono::system_clock::now();
            elapsedTimeFlag = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimeFlag - iterator->second);

            std::string elapsedTimeFlagString = std::to_string(elapsedTimeFlag.count() / 1000.0) + "\b\b\b seconds."; // \b for 3 digits

			logINFO("Flag '", flagName, "' released after ", elapsedTimeFlagString);
        }
    }

private:
    template <typename... Args>
    INLINING_TINYLOGGER std::string formatMessage(Args&&... args) const {

        // Computing and preparing the time string
        currentTime_ = std::chrono::system_clock::now(); // Used to compute elapsed time since last log
        elapsedTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime_ - lastLogTime_);

        currentTimeInstanced_ = std::chrono::system_clock::to_time_t(currentTime_); // Printing current time
        #ifdef _WIN32 // ctime_s is used for Windows compatibility
            ctime_s(currentTimeChar_, sizeof(currentTimeChar_), &currentTimeInstanced_); currentTimeChar_[24] = '\0';
        #else         // ctime_r is used for Linux compatibility
            ctime_r(&currentTimeInstanced_, currentTimeChar_); currentTimeChar_[24] = '\0';
        #endif        // Both are thread-safe while using ctime only is not

        elapsedTimeString_ = " +" + std::to_string(elapsedTime_.count() / 1000.0) + "\b\b\b s "; // \b for 3 digits

		// Log arguments are concatenated into a single string
        std::ostringstream streamMessage;
        (streamMessage << ... << std::forward<Args>(args));

        std::string stringMessage = streamMessage.str();

		// Saving the current time for the next log
        lastLogTime_ = currentTime_;

		return std::string(currentTimeChar_) + elapsedTimeString_ + stringMessage;
    }

    LogLevel logLevel_;

    // Used to compute time between every logging event
    mutable std::chrono::system_clock::time_point currentTime_;
    mutable std::chrono::system_clock::time_point lastLogTime_;
	mutable std::chrono::milliseconds elapsedTime_ = std::chrono::milliseconds(0);

    // Used in order to print current and elapsed time
    mutable time_t currentTimeInstanced_; 
    mutable char   currentTimeChar_[26];
    mutable std::string elapsedTimeString_;

    // Used to update a progress bar for current status
    mutable std::atomic<size_t> currentIteration_{ 0 };
    mutable std::atomic<size_t> numberIterations_{ 1 };

    // Flags can be added to memory to track specific time events
    mutable std::unordered_map<std::string, std::chrono::system_clock::time_point> flagTimes_;

    // Mutex added for thread-safety
	mutable std::mutex flagMutex_; // Mutex to synchronize flags access
	mutable std::mutex logMutex_;  // Mutex to synchronize logger access
};


/*
 * Inline logger instance that can be used in the code, originally defined with
 * a LogLevel::TRACE log level. This logger instance is used by the macros, and
 * can then be called in all the program.
 *  /!\ Caution: is strongly recommended to work using the macros that directly
 *      call the logging functions, rather than using the functions themselves.
 *      Using the macros can greatly optimise both code performance, and binary
 *      size.
 */
inline Logger logger(LogLevel::TRACE);


/*
 * Generates a sequence of context information arguments to be prepended to the
 * log messages, depending on compile-time flags. In order, this macro includes
 * the following, in order:
 *  - Function name: in which the log message is generated if LOG_FUNCTION_NAME
 *  - File name    : in which the log message is generated if LOG_FILE_NAME
 *  - Line number  : in which the log message is generated if LOG_LINE_NUMBER
 * 
 * These components are passed as separate arguments to the macro associated to
 * the variadic logger function, which is responsible for concatenating them.
 */

#define   STR(x)    #x
#define TOSTR(x) STR(x)

#if    LOG_FUNCTION_NAME &&  LOG_FILE_NAME &&  LOG_LINE_NUMBER
#   define LOG_CONTEXT() __FUNCTION__, ": in [", __FILE__, "] (l. ", TOSTR(__LINE__), ") "
#elif  LOG_FUNCTION_NAME &&  LOG_FILE_NAME && !LOG_LINE_NUMBER
#   define LOG_CONTEXT() __FUNCTION__, ": in [", __FILE__, "] "
#elif  LOG_FUNCTION_NAME && !LOG_FILE_NAME &&  LOG_LINE_NUMBER
#   define LOG_CONTEXT() __FUNCTION__, ": "
#elif  LOG_FUNCTION_NAME && !LOG_FILE_NAME && !LOG_LINE_NUMBER
#   define LOG_CONTEXT() __FUNCTION__, ": (l. ", TOSTR(__LINE__), ") "
#elif !LOG_FUNCTION_NAME &&  LOG_FILE_NAME &&  LOG_LINE_NUMBER
#   define LOG_CONTEXT() " in [", __FILE__, "] (l. ", TOSTR(__LINE__), ") "
#elif !LOG_FUNCTION_NAME &&  LOG_FILE_NAME && !LOG_LINE_NUMBER
#   define LOG_CONTEXT() " in [", __FILE__, "] "
#elif !LOG_FUNCTION_NAME && !LOG_FILE_NAME &&  LOG_LINE_NUMBER
#   define LOG_CONTEXT() " (l. ", TOSTR(__LINE__), ") "
#else
#   define LOG_CONTEXT() ""
#endif


/* =========================================================================
 *           Logger Macros Optimisation at Compilation Time
 * =========================================================================
 * 
 * If the flag MAX_LOG_LEVEL_AT_COMPILATION is defined at compilation, the
 * macros will be redefined, and optimised to call the associated function
 * directly, without the need to use the log function as intermediary, and
 * also without the need to check condition and enter the switch.
 */
#ifndef MAX_LOG_LEVEL_AT_COMPILATION
   /*
    * @brief Logs a message at the TRACE severity level.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
    */
#   define LOG_TRACE(...)    logger.log(LogLevel::TRACE   , LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the DEBUG severity level.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
    */
#   define LOG_DEBUG(...)    logger.log(LogLevel::DEBUG   , LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the VERBOSE severity level.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_VERBOSE(...)  logger.log(LogLevel::VERBOSE , LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the INFO severity level.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_INFO(...)     logger.log(LogLevel::INFO    , LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the WARNING severity level.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_WARNING(...)  logger.log(LogLevel::WARNING , LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the LERROR severity level.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_ERROR(...)    logger.log(LogLevel::LERROR  , LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the CRITICAL severity level. This exits program.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_CRITICAL(...) logger.log(LogLevel::CRITICAL, LOG_CONTEXT(), __VA_ARGS__)
#else
   /*
    * @brief Logs a message at the TRACE severity level. Is optimised to bypass
	*        all the checks and directly call the log function.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
    */
#   define LOG_TRACE(...)    logger.logTRACE(   LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the DEBUG severity level. Is optimised to bypass
    *        all the checks and directly call the log function.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_DEBUG(...)    logger.logDEBUG(   LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the VERBOSE severity level. Is optimised to bypass
    *        all the checks and directly call the log function.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_VERBOSE(...)  logger.logVERBOSE( LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the INFO severity level. Is optimised to bypass
    *        all the checks and directly call the log function.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_INFO(...)     logger.logINFO(    LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the WARNING severity level. Is optimised to bypass
    *        all the checks and directly call the log function.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_WARNING(...)  logger.logWARNING( LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the ERROR severity level. Is optimised to bypass
    *        all the checks and directly call the log function.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_ERROR(...)    logger.logERROR(   LOG_CONTEXT(), __VA_ARGS__)
   /*
    * @brief Logs a message at the CRITICAL severity level. Is optimised to bypass
    *        all the checks and directly call the log function. This exits program.
    *
    * @note If MAX_LOG_LEVEL_AT_COMPILATION is defined to a lower level calling
    *       this macro may be entirely removed at compile-time for optimisation
    *
    * @param ... (variadic): Any arguments of any streamable type, concatenated
    *                        into the log message.
	*/
#   define LOG_CRITICAL(...) logger.logCRITICAL(LOG_CONTEXT(), __VA_ARGS__)
#endif


/*
 * =========================================================================
 *                          Logger Header Cleanup
 * =========================================================================
 * 
 * In this section, we intentionally use "#undef" to remove macros that were
 * previously defined in this header. This will be made depending on a value
 * of MAX_LOG_LEVEL_AT_COMPILATION flag. With this method, we can reduce the
 * size of the final binary by removing the unused logging macros, and, that
 * is more important here, we can optimise the code performance.
 * On the other hand we lose the ability to choose the log level at runtime.
 */


#ifndef    MAX_LOG_LEVEL_AT_COMPILATION
#   define MAX_LOG_LEVEL_AT_COMPILATION 7
#endif


#if (MAX_LOG_LEVEL_AT_COMPILATION < 7)
#   undef  LOG_TRACE
    // Empty macro since the flag MAX_LOG_LEVEL_AT_COMPILATION is set below 7
#   define LOG_TRACE(...)
#endif

#if (MAX_LOG_LEVEL_AT_COMPILATION < 6)
#   undef  LOG_DEBUG
    // Empty macro since the flag MAX_LOG_LEVEL_AT_COMPILATION is set below 6
#   define LOG_DEBUG(...)
#endif

#if (MAX_LOG_LEVEL_AT_COMPILATION < 5)
#   undef  LOG_VERBOSE
    // Empty macro since the flag MAX_LOG_LEVEL_AT_COMPILATION is set below 5
#   define LOG_VERBOSE(...)
#endif

#if (MAX_LOG_LEVEL_AT_COMPILATION < 4)
#   undef  LOG_INFO
    // Empty macro since the flag MAX_LOG_LEVEL_AT_COMPILATION is set below 4
#   define LOG_INFO(...)
#endif

#if (MAX_LOG_LEVEL_AT_COMPILATION < 3)
#   undef  LOG_WARNING
    // Empty macro since the flag MAX_LOG_LEVEL_AT_COMPILATION is set below 3
#   define LOG_WARNING(...)
#endif

#if (MAX_LOG_LEVEL_AT_COMPILATION < 2)
#   undef  LOG_ERROR
    // Empty macro since the flag MAX_LOG_LEVEL_AT_COMPILATION is set below 2
#   define LOG_ERROR(...)
#endif

#if (MAX_LOG_LEVEL_AT_COMPILATION < 1)
#   undef  LOG_CRITICAL
    // Empty macro since the flag MAX_LOG_LEVEL_AT_COMPILATION is set below 1
#   define LOG_CRITICAL(...)
#endif