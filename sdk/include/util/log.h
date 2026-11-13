#pragma once
#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <mutex>

namespace Jammy
{
    class Logger
    {
    public:
    // 问：为什么需要日志名称
        static void LoggerInit(const std::string& logger_name, const std::string& file_name,spdlog::level::level_enum logger_level);
        static std::shared_ptr<spdlog::logger> GetLogger();
    // 问：为什么要将构造函数私有
    private:
        Logger();
        // 问：为什么要禁掉拷贝构造和赋值
        Logger& operator=(const Logger&) = delete;
        Logger(const Logger&) = delete;
    private:
        // 问：为什么这里要使用static
        static std::shared_ptr<spdlog::logger> g_logger;
        static std::mutex _mutex; 
    };

// 问：{}是什么意思
#define FORMAT std::string("[{}:{}]")
// 问：##__VA_ARGS__ 有什么用
// 问：宏只是将上层的字符串拼接起来，然后传递给spdlog然后spdlog自动根据文件内容打印到相应的位置吗
#define DBGUG(fmt,...) Logger::GetLogger()->debug(FORMAT + fmt,__FILE__,__LINE__,##__VA_ARGS__)
#define INFO(fmt,...) Logger::GetLogger()->info(FORMAT + fmt,__FILE__,__LINE__,##__VA_ARGS__)
#define ERROR(fmt,...) Logger::GetLogger()->error(FORMAT + fmt,__FILE__,__LINE__,##__VA_ARGS__)
#define CRIT(fmt,...) Logger::GetLogger()->critical(FORMAT + fmt,__FILE__,__LINE__,##__VA_ARGS__)
#define WARNING(fmt,...) Logger::GetLogger()->warn(FORMAT + fmt,__FILE__,__LINE__,##__VA_ARGS__)
}