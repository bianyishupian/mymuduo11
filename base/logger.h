#pragma once
#include <string>
#include "noncopyable.h"

#define LOG_INFO(logmsgFormat,...)                          \
    do                                                      \
    {                                                       \
        Logger &logger = Logger::instance();                \
        logger.setLogLevel(INFO);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while (0);                                            \

#define LOG_ERROR(logmsgFormat,...)                         \
    do                                                      \
    {                                                       \
        Logger &logger = Logger::instance();                \
        logger.setLogLevel(ERROR);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while (0);                                            \

#define LOG_FATAL(logmsgFormat,...)                          \
    do                                                      \
    {                                                       \
        Logger &logger = Logger::instance();                \
        logger.setLogLevel(FATAL);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while (0);                                            \

#define LOG_DEBUG(logmsgFormat,...)                          \
    do                                                      \
    {                                                       \
        Logger &logger = Logger::instance();                \
        logger.setLogLevel(DEBUG);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while (0);                                            \




enum LogLevel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

class Logger : noncopyable
{
public:
    static Logger &instance();  // 单例模式
    void setLogLevel(int level);
    void log(std::string msg);
private:
    int _level;
};