#include "ChatServer.h"
#include <gflags/gflags.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <chrono>
#include <thread>
#include <spdlog/common.h>
#include <ai_chat_sdk/util/log.h>
using namespace Jammy;

// ---------------- 命令行参数定义 ----------------
DEFINE_string(host, "0.0.0.0", "服务器绑定的地址");
DEFINE_int32(port, 8080, "服务器绑定的端口号");
DEFINE_string(log_level, "INFO", "日志级别: TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL");
DEFINE_double(temperature, 0.7, "温度值，影响生成文本的随机性 (0.0 ~ 2.0)");
DEFINE_int32(max_tokens, 2048, "最大 token 数，必须为正整数");
DEFINE_string(config_file, "", "配置文件路径（可选，文件中的参数会被命令行参数覆盖）");

// Ollama 配置（云端模型 apikey 从环境变量读，不需要命令行参数）
DEFINE_string(ollama_model_name, "deepseek-r1:1.5b", "Ollama 模型名称，例如 deepseek-r1:1.5b");
DEFINE_string(ollama_model_desc, "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理", "Ollama 模型描述");
DEFINE_string(ollama_endpoint, "http://127.0.0.1:11434", "Ollama API 地址，例如 http://127.0.0.1:11434");

// 版本号
const std::string VERSION = "1.0.0";

// ---------------- 工具函数 ----------------

// 安全获取环境变量：返回空字符串而非 nullptr
std::string getEnvVar(const std::string& key)
{
    const char* value = std::getenv(key.c_str());
    return value ? std::string(value) : "";
}

// ---------------- 配置校验 ----------------

bool validateConfig(ai_chat_server::ChatServerConfig& config)
{
    // 温度值范围 0.0 ~ 2.0
    if (config._temperature < 0.0 || config._temperature > 2.0)
    {
        ERROR("validateConfig: 温度值必须在 0.0 ~ 2.0 之间，当前值: {}", config._temperature);
        return false;
    }

    // 最大 token 必须为正整数
    if (config._max_token <= 0)
    {
        ERROR("validateConfig: 最大 token 必须为正整数，当前值: {}", config._max_token);
        return false;
    }

    // 端口合法性
    if (config._port <= 0 || config._port > 65535)
    {
        ERROR("validateConfig: 端口号必须在 1 ~ 65535 之间，当前值: {}", config._port);
        return false;
    }

    // 至少有一个 API key 不为空（云端模型），或者 Ollama 模型名不为空
    bool hasCloudKey = !config.deepseek_apikey.empty() ||
                       !config.chatgpt_apikey.empty() ||
                       !config.gemini_apikey.empty();
    bool hasOllama = !config._ollama_model.empty();

    if (!hasCloudKey && !hasOllama)
    {
        ERROR("validateConfig: 至少需要提供一个云端模型的 API Key，或配置 Ollama 模型");
        ERROR("  - 可通过环境变量设置: DeepSeek_apikey / ChatGPT_apikey / Gemini_apikey");
        ERROR("  - 或通过命令行设置: --ollama_model_name / --ollama_endpoint");
        return false;
    }

    // 如果提供了 Ollama 模型名，则 endpoint 也必须提供
    if (!config._ollama_model.empty() && config._ollama_endpoint.empty())
    {
        ERROR("validateConfig: 提供了 Ollama 模型名但缺少 --ollama_endpoint");
        return false;
    }

    return true;
}

// ---------------- 帮助 / API 说明 ----------------

void showUsage()
{
    std::cout << "AIChatServer v" << VERSION << "\n\n";
    std::cout << "用法: ./AIChatServer [选项]\n\n";
    std::cout << "选项:\n";
    std::cout << "  --host=<ip>                服务器绑定地址，默认 0.0.0.0\n";
    std::cout << "  --port=<port>              服务器绑定端口，默认 8080\n";
    std::cout << "  --log_level=<level>        日志级别: TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL，默认 INFO\n";
    std::cout << "  --temperature=<value>      温度值 0.0~2.0，默认 0.7\n";
    std::cout << "  --max_tokens=<num>         最大 token 数，默认 2048\n";
    std::cout << "  --config_file=<path>       配置文件路径（可选）\n";
    std::cout << "  --ollama_model_name=<name> Ollama 模型名称，例如 deepseek-r1:1.5b\n";
    std::cout << "  --ollama_model_desc=<desc> Ollama 模型描述\n";
    std::cout << "  --ollama_endpoint=<url>    Ollama API 地址，例如 http://127.0.0.1:11434\n";
    std::cout << "  -h, --help                 显示本帮助并退出\n";
    std::cout << "  -v, --version              显示版本号并退出\n";
    std::cout << "\n";
    std::cout << "环境变量:\n";
    std::cout << "  DeepSeek_apikey            DeepSeek API Key\n";
    std::cout << "  ChatGPT_apikey             ChatGPT API Key\n";
    std::cout << "  Gemini_apikey              Gemini API Key\n";
    std::cout << "\n";
    std::cout << "使用示例:\n";
    std::cout << "  # 默认启动（从环境变量读 apikey）\n";
    std::cout << "  export DeepSeek_apikey=sk-xxx\n";
    std::cout << "  ./AIChatServer\n";
    std::cout << "\n  # 指定端口 + Ollama\n";
    std::cout << "  ./AIChatServer --port=9000 --ollama_model_name=deepseek-r1:1.5b --ollama_endpoint=http://127.0.0.1:11434\n";
    std::cout << "\n  # 使用配置文件\n";
    std::cout << "  ./AIChatServer --config_file=ChatServer.conf\n";
    std::cout << "\n";
}

void showAPIInfo()
{
    std::cout << "\nChatServer API 接口说明:\n\n";
    std::cout << "  POST   /api/session                    创建新会话\n";
    std::cout << "          body: {\"model\": \"deepseek-flash\"}\n";
    std::cout << "          返回: {\"success\": true, \"data\": {\"session_id\": \"...\"}}\n\n";
    std::cout << "  GET    /api/sessions                   获取所有会话列表\n";
    std::cout << "          返回: {\"success\": true, \"data\": [...]}\n\n";
    std::cout << "  GET    /api/models                     获取可用模型列表\n";
    std::cout << "          返回: {\"success\": true, \"data\": [{\"name\": \"...\", \"desc\": \"...\"}]}\n\n";
    std::cout << "  DELETE /api/session/{session_id}       删除指定会话\n";
    std::cout << "          返回: {\"success\": true/false}\n\n";
    std::cout << "  GET    /api/session/{session_id}/history  获取会话历史消息\n";
    std::cout << "          返回: {\"success\": true, \"data\": [{\"role\": \"...\", \"content\": \"...\"}]}\n\n";
    std::cout << "  POST   /api/message                    发送消息（全量返回）\n";
    std::cout << "          body: {\"session_id\": \"...\", \"message\": \"你好\"}\n";
    std::cout << "          返回: {\"success\": true, \"data\": {\"response\": \"...\"}}\n\n";
    std::cout << "  POST   /api/message/async              发送消息（SSE 流式返回）\n";
    std::cout << "          body: {\"session_id\": \"...\", \"message\": \"你好\"}\n";
    std::cout << "          返回: SSE text/event-stream 格式\n\n";
}

// ---------------- 主函数 ----------------

int main(int argc, char** argv)
{
    try
    {
        // ---- 1. 处理 -h/-v 的早退出（gflags 自己也会处理，但我们要追加 API 说明） ----
        if (argc == 2)
        {
            std::string arg(argv[1]);
            if (arg == "-h" || arg == "--help")
            {
                showUsage();
                showAPIInfo();
                return 0;
            }
            if (arg == "-v" || arg == "--version")
            {
                std::cout << "AIChatServer v" << VERSION << std::endl;
                return 0;
            }
        }

        // ---- 2. 解析命令行参数 ----
        gflags::SetUsageMessage("AIChatServer - AI 聊天服务器\n\n用法: ./AIChatServer [选项]");
        gflags::SetVersionString(VERSION);
        gflags::ParseCommandLineFlags(&argc, &argv, true);

        // ---- 3. 加载配置文件（如果指定） ----
        // gflags 的 flagfile 机制：--flagfile=ChatServer.conf
        // 配置文件每行格式与命令行相同，例如：--port=9000
        if (!FLAGS_config_file.empty())
        {
            std::ifstream confFile(FLAGS_config_file);
            if (confFile.good())
            {
                // 让 gflags 重新用配置文件解析，命令行参数优先级更高
                gflags::SetCommandLineOption("flagfile", FLAGS_config_file.c_str());
                INFO("main: 加载配置文件成功: {}", FLAGS_config_file);
            }
            else
            {
                WARNING("main: 配置文件不存在: {}，跳过加载", FLAGS_config_file);
            }
        }

        // ---- 4. 构建 ChatServerConfig ----
        ai_chat_server::ChatServerConfig config;
        config._ip = FLAGS_host;
        config._port = FLAGS_port;
        config._log_level = FLAGS_log_level;
        config._temperature = FLAGS_temperature;
        config._max_token = FLAGS_max_tokens;

        // 云端 API key 从环境变量读取
        config.deepseek_apikey = getEnvVar("DeepSeek_apikey");
        config.chatgpt_apikey = getEnvVar("ChatGPT_apikey");
        config.gemini_apikey = getEnvVar("Gemini_apikey");

        // Ollama 配置从命令行参数读取
        config._ollama_model = FLAGS_ollama_model_name;
        config._ollama_desc = FLAGS_ollama_model_desc;
        config._ollama_endpoint = FLAGS_ollama_endpoint;

        // ---- 5. 参数校验 ----
        if (!validateConfig(config))
        {
            ERROR("main: 配置校验失败，启动终止");
            return 1;
        }

        // ---- 6. 初始化日志 ----
        spdlog::level::level_enum logLevel = spdlog::level::info;
        if      (config._log_level == "TRACE")   logLevel = spdlog::level::trace;
        else if (config._log_level == "DEBUG")   logLevel = spdlog::level::debug;
        else if (config._log_level == "INFO")    logLevel = spdlog::level::info;
        else if (config._log_level == "WARN" ||
                 config._log_level == "WARNING") logLevel = spdlog::level::warn;
        else if (config._log_level == "ERROR")   logLevel = spdlog::level::err;
        else if (config._log_level == "CRITICAL") logLevel = spdlog::level::critical;
        else
        {
            WARNING("main: 未知日志级别 '{}'，使用默认 INFO", config._log_level);
        }
        Jammy::Logger::LoggerInit("ChatServer", "stdout", logLevel);

        // ---- 7. 打印启动配置（脱敏） ----
        INFO("========================================");
        INFO("  AIChatServer v{} 启动中...", VERSION);
        INFO("========================================");
        INFO("  监听地址:      {}:{}", config._ip, config._port);
        INFO("  日志级别:      {}", config._log_level);
        INFO("  温度值:        {}", config._temperature);
        INFO("  最大 Token:    {}", config._max_token);
        INFO("  DeepSeek Key:  {}", config.deepseek_apikey.empty() ? "未设置" : "已设置");
        INFO("  ChatGPT Key:   {}", config.chatgpt_apikey.empty()  ? "未设置" : "已设置");
        INFO("  Gemini Key:    {}", config.gemini_apikey.empty()   ? "未设置" : "已设置");
        INFO("  Ollama 模型:   {}", config._ollama_model.empty()  ? "未设置" : config._ollama_model);
        INFO("  Ollama 端点:   {}", config._ollama_endpoint.empty()? "未设置" : config._ollama_endpoint);
        INFO("========================================");

        // ---- 8. 创建并启动服务器 ----
        INFO("main: ChatServerConfig 构建完成: ip={}, port={}, temperature={}, max_token={}", 
             config._ip, config._port, config._temperature, config._max_token);
        INFO("main: 开始构造 ChatServer...");
        ai_chat_server::ChatServer server(config);
        INFO("main: ChatServer 构造完成");
        if (!server.start())
        {
            ERROR("main: ChatServer 启动失败");
            return 1;
        }

        INFO("ChatServer 启动成功! http://{}:{}", config._ip, config._port);
        INFO("输入 Ctrl+C 停止服务器");

        // ---- 9. 主线程阻塞等待 ----
        // ChatServer::start() 在独立线程里 listen，主线程循环等待直到 stop() 被调用
        while (server.isAvailable())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        INFO("ChatServer 已停止");
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "main: 发生异常: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "main: 发生未知异常" << std::endl;
        return 1;
    }
}
