/**
 * Logger.cpp - a printf-like logger
 */

#include <cstdarg>
#include <cstring>

#include <thread>

#include "Logger.hpp"

#ifdef LOCK_LOGGER
#include <mutex>

static std::mutex write_mtx;
#endif

/**
 * Translates a level into a readable string.
 *
 * @param level the level
 *
 * @return the level as a string
 */
static const char * strlevel(Slog::Level level) {
    switch(level) {
        case Slog::Level::TRACE:
            return "trace";
        case Slog::Level::DEBUG:
            return "debug";
        case Slog::Level::INFO:
            return "info";
        case Slog::Level::NOTICE:
            return "note ";
        case Slog::Level::WARNING:
            return "WARN";
        case Slog::Level::ERROR:
            return "ERROR";
        case Slog::Level::CRITICAL:
            return "CRIT";
        case Slog::Level::ALERT:
            return "ALERT";
        case Slog::Level::EMERGENCY:
            return "EMERG";
        default:
            return "OH NO";
    }
}

#define MAP(lvl_) if(strncmp(levelString, #lvl_, strlen(#lvl_)) == 0) return Slog::Level::lvl_

static Slog::Level map_level(const char * lvlString) {
    std::string copy = lvlString;

    // Convert to upper case
    size_t i;
    for(i = 0; i < copy.size(); ++i) copy[i] = static_cast<char>(toupper(copy[i]));

    const auto levelString = copy.c_str();

    MAP(NONE);
    MAP(TRACE);
    MAP(DEBUG);
    MAP(INFO);
    MAP(NOTICE);
    MAP(NOTE);
    MAP(WARNING);
    MAP(WARN);
    MAP(ERROR);
    MAP(ERR);
    MAP(CRIT);
    MAP(CRITICAL);
    MAP(ALERT);
    MAP(EMERG);
    MAP(EMERGENCY);
    MAP(ALL);

    throw Slog::InvalidLevelException(lvlString);
}

#undef MAP

void Slog::Logger::set_level(const std::string &name) {
    this->level = map_level(name.c_str());
}

void Slog::Logger::log(Level level, const char *format, ...) const {
    // Check mininum level
    if(level < this->level) return;

    va_list args;
    va_start(args, format);

    // First determine the time and its representation
    time_t timeInfo = time(nullptr);
    char timeString[128];
    size_t timeStringLength = strftime(
        timeString, 128, this->date_time_format.c_str(), localtime(&timeInfo)
    );

    // Format the message
    va_list copy;
    va_copy(copy, args);
    size_t messageLength = static_cast<size_t>(vsnprintf(nullptr, 0, format, copy) + 1);
    va_end(copy);

    std::vector<char> message(messageLength);
    vsnprintf(message.data(), messageLength, format, args);

    // Generate the log entry
    size_t outputLength =
        timeStringLength + this->name.length() + messageLength + strlen("[] [] [FFFF] [oh no] \n");
    std::vector<char> output(outputLength);

    outputLength = (size_t) snprintf(
        output.data(), outputLength, "[%s] [%s] [%04X] [%-5s] %s\n",
        timeString, this->name.c_str(),
        static_cast<int>(std::hash<std::thread::id>()(std::this_thread::get_id()) & 0xFFFF),
        strlevel(level), message.data()
    );
    // Prevent the threads from writing all over each other
    {
#ifdef LOCK_LOGGER
        write_mtx.lock();
#endif

        // Print the entry to all targets
        for (auto &target : this->targets) {
            fwrite(output.data(), 1, outputLength, target);
        }

#ifdef LOCK_LOGGER
        write_mtx.unlock();
#endif
    }

    va_end(args);
}
