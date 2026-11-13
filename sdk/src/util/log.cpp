#include "../../include/util/log.h"

namespace Jammy
{
    // 问：为什么要在这里构造
    Logger::Logger()
    {}

    // 问：私有成员也可以通过Logger进行访问吗
    // 问：为什么不在.h文件中定义
    std::shared_ptr<spdlog::logger> Logger::g_logger = nullptr;
    std::mutex Logger::_mutex;

    void Logger::LoggerInit(const std::string& logger_name, const std::string& file_name,spdlog::level::level_enum logger_level)
    {
        // 问：为什么不加Logger就能访问
        if(g_logger == nullptr)  // 来判断是否创建了日志对象
        {
            std::lock_guard<std::mutex> lock(_mutex);
            // 问：为什么需要两个if判断
            if(g_logger == nullptr)
            {
                // 问：为什么要刷新日志等级
                spdlog::flush_on(logger_level);   // 刷新日志等级

                spdlog::init_thread_pool(32768,1);  // 参数一：队列大小，参数二：线程个数
                if(file_name == "stdout")
                {
                    // 问：为什么这里不加<spdlog::async_factory>
                    g_logger = spdlog::stdout_color_mt(logger_name);
                }
                else
                {
                    g_logger = spdlog::basic_logger_mt<spdlog::async_factory>(logger_name,file_name);
                }

                spdlog::set_level(logger_level);
                // 配置⽇志输出格式：
                // %H:%M:%S 时分秒
                // %n ⽇志器的名称
                // %l: ⽇志级别的名字info、error，%-7l：左对⻬占7个字符
                // %v: ⽇志消息正⽂
                spdlog::set_pattern("[%H:%M:%S][%n][%-7l]: %v");
            }
        }
    }

    std::shared_ptr<spdlog::logger> Logger::GetLogger()
    {
        return g_logger;
    }
}