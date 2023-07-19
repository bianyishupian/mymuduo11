#include <iostream>

#include "logger.h"
#include "timestamp.h"

Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level)
{
    _level = level;
}

void Logger::log(std::string msg)
{
    std::string pre = "";
    switch (_level)
    {
    case INFO:
        pre = "[INFO]";
        break;
    case ERROR:
        pre = "[ERROR]";
        break;
    case FATAL:
        pre = "[FATAL]";
        break;
    case DEBUG:
        pre = "[DEBUG]";
        break;
    default:
        break;
    }

    std::cout << pre + Timestamp::now().toString() << " : " << msg << std::endl;
}