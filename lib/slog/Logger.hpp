/**
 * Logger.h - a printf-like logger
 */

#pragma once

#include <cstdio>
#include <climits>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef DEBUG
#undef DEBUG
#endif

namespace Slog {

/**
* Logging levels.
*/
enum Level {
    TRACE     = 0,
    DEBUG     = 1,
    INFO      = 2,
    NOTICE    = 3,
    NOTE      = 3,
    WARNING   = 4,
    WARN      = 4,
    ERROR     = 5,
    ERR       = 5,
    CRIT      = 6,
    CRITICAL  = 6,
    ALERT     = 7,
    EMERG     = 8,
    EMERGENCY = 8,

    ALL       = INT_MIN,
    NONE      = INT_MAX
};

/**
 * An exception signaling an invalid level name or number.
 */
class InvalidLevelException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * The usual logger.
 */
class Logger {
private:
    /**
     * The name of the logger.
     */
    std::string name;

    /**
     * The targets to write the entry to.
     */
    std::vector<FILE *> targets;

    /**
     * The format of the datetime that's printed before every entry.
     */
    std::string date_time_format;

    /**
     * The minimum logging level, below this the entry will not get printed.
     */
    Level level;

public:
    /**
     * Creates a new logger.
     *
     * @param name          the name of the logger
     * @param initialLevel  the initial level for the logger, defaults to INFO
     * @param initialTarget the initial target for the logger, defaults to stderr
     */
    Logger(const std::string &name, Level initialLevel = INFO, FILE *initialTarget = stderr) {
        this->name = name;
        this->targets.push_back(initialTarget);
        this->date_time_format = "%Y-%m-%d %H:%M:%S %z";
        this->level = initialLevel;
    }

    /**
     * Creates a new logger by cloning another.
     *
     * @param other the logger to clone
     * @param name  the name of the cloned logger
     */
    Logger(const Logger &other, const std::string &name) {
        this->name = name;
        this->targets = other.targets;
        this->date_time_format = other.date_time_format;
        this->level = other.level;
    }

    /**
     * Logs a message.
     *
     * @param level  the level the message is at
     * @param format the format of the message
     * @param ...    any arguments
     */
    void log(const Level level, const char *format, ...) const;

    /**
     * Logs a message with the trace level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void trace(const char *format, P... args) const {
#ifndef NDEBUG
        this->log(Level::TRACE, format, args...);
#else
        (void) format;
#endif
    }

    /**
     * Logs a message with the debug level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void debug(const char *format, P... args) const {
#ifndef NDEBUG
        this->log(Level::DEBUG, format, args...);
#else
        (void) format;
#endif
    }

    /**
     * Logs a message with the info level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void info(const char *format, P... args) const {
        this->log(Level::INFO, format, args...);
    }

    /**
     * Logs a message with the notice level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void notice(const char *format, P... args) const {
        this->log(Level::NOTICE, format, args...);
    }

    /**
     * Logs a message with the note level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void note(const char *format, P... args) const {
        this->log(Level::NOTE, format, args...);
    }

    /**
     * Logs a message with the warning level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void warning(const char *format, P... args) const {
        this->log(Level::WARNING, format, args...);
    }

    /**
     * Logs a message with the warn level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void warn(const char *format, P... args) const {
        this->log(Level::WARN, format, args...);
    }

    /**
     * Logs a message with the error level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void error(const char *format, P... args) const {
        this->log(Level::ERROR, format, args...);
    }

    /**
     * Logs a message with the err level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void err(const char *format, P... args) const {
        this->log(Level::ERR, format, args...);
    }

    /**
     * Logs a message with the crit level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void crit(const char *format, P... args) const {
        this->log(Level::CRIT, format, args...);
    }

    /**
     * Logs a message with the critical level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void critical(const char *format, P... args) const {
        this->log(Level::CRITICAL, format, args...);
    }

    /**
     * Logs a message with the alert level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void alert(const char *format, P... args) const {
        this->log(Level::ALERT, format, args...);
    }

    /**
     * Logs a message with the emerg level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void emerg(const char *format, P... args) const {
        this->log(Level::EMERG, format, args...);
    }

    /**
     * Logs a message with the emergency level.
     *
     * @param format the format of the message
     * @param ...    any arguments
     */
    template <typename... P>
    void emergency(const char *format, P... args) const {
        this->log(Level::EMERGENCY, format, args...);
    }

    /**
     * Adds a target to the logger.
     *
     * @param target the target to add
     */
    void add_target(FILE *target) {
        this->targets.push_back(target);
    }

    /**
     * Removes a target from the logger.
     *
     * @param target the target to remove
     */
    void remove_target(FILE *target) {
        this->targets.erase(
            std::remove(this->targets.begin(), this->targets.end(), target),
            this->targets.end()
        );
    }

    /**
     * Sets the logger's minimum level.
     *
     * @param level the level to set
     */
    void set_level(Level level) {
        this->level = level;
    }

    /**
     * Sets the logger's minimum level by name.
     *
     * @param name the level to set
     */
    void set_level(const std::string &name);

    /**
     * Returns the logger's minimum level.
     *
     * @return the level
     */
    Level get_level() const {
        return this->level;
    }
};
}
