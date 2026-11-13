#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace std;

shared_ptr<spdlog::logger> g_logger;

struct setting
{
    bool async = false;
    int level = 1;
    string format = "[%H:%M:%S] [%-7l] %v";
    string path = "stdout";
};

void mylog(const setting& set = setting())
{
    if(set.async == false)
    {
        if(set.path == "stdout")
        {
            g_logger = spdlog::stdout_color_mt<spdlog::async_factory>("stdout_log");
        }
        else
        {
             g_logger = spdlog::basic_logger_mt<spdlog::async_factory>("file_logger",set.path);
        }
    }
    else
    {
        if(set.path == "stdout")
        {
            g_logger = spdlog::stdout_color_mt<spdlog::async_factory>("stdout_log");
        }
        else
        {
             g_logger = spdlog::basic_logger_mt<spdlog::async_factory>("file_logger",set.path);
        }
    }

    g_logger->set_level(spdlog::level::level_enum(set.level));
    g_logger->set_pattern(set.format);
}

#define FMORT string("[{}:{}] : ")
#define DBG(fmt,...) g_logger->debug(FMORT + fmt,__FILE__,__LINE__,##__VA_ARGS__)
#define INF(fmt,...) g_logger->info(FMORT + fmt,__FILE__,__LINE__,##__VA_ARGS__)
#define ERR(fmt,...) g_logger->error(FMORT + fmt,__FILE__,__LINE__,##__VA_ARGS__)
#define WRN(fmt,...) g_logger->warn(FMORT + fmt,__FILE__,__LINE__,##__VA_ARGS__)

int main()
{
    //4. 初始化⽇志器
    mylog();
    DBG("{}今年{}岁", "⼩明", 18);
    INF("{}今年{}岁", "⼩红", 19);
    WRN("{}今年{}岁", "⼩刚", 20);
    ERR("{}今年{}岁", "⼩李", 21);
    return 0;
}