#pragma once
#include <httplib.h>
#include <memory>
#include <string>
#include <ai_chat_sdk/ChatSDK.h>

namespace ai_chat_server
{
    // 构建一个类来存放相关配置信息
    struct ChatServerConfig
    {
        // 服务器相关配置文件
        std::string _ip = "0.0.0.0";   // 服务器IP地址
        int _port = 8080;              // 服务器端口号
        std::string _log_level = "INFO"; // 日志级别

        // 公共配置
        double _temperature = 0.7;  // 模型设置的温度，控制模型回复的准确度
        int _max_token = 2048;  // 输入的最大token数

        // api模型
        // 问：为什么api类的模型不需要模型名
        std::string deepseek_apikey;
        std::string chatgpt_apikey;
        std::string gemini_apikey;

        // ollama模型
        std::string _ollama_model;
        std::string _ollama_endpoint;
        std::string _ollama_desc;
    };
    
    class ChatServer
    {
    public:
        ChatServer(const ChatServerConfig& config);
        // 启动服务器
        bool start();
        // 停止服务器
        bool stop();
        // 判断服务器是否存活
        bool isAvailable() const;
    // 构建接口，用于处理http请求和响应
    private:
        // 设置路由规则
        void setRoutes();
        // 构建错误回应
        std::string buildResponse(const std::string& message, bool success);
        // 处理获取会话列表的请求
        void handleGetSessionLists(const httplib::Request& request, httplib::Response& response);
        // 处理获取可用大模型列表的请求
        void handleGetModelLists(const httplib::Request& request, httplib::Response& response);
        // 处理创建会话的请求
        void handleCreateSession(const httplib::Request& request, httplib::Response& response);
        // 处理删除会话的请求
        void handleDeleteSession(const httplib::Request& request, httplib::Response& response); 
        // 处理获取历史消息的请求
        void handleGetHistoryMessage(const httplib::Request& request, httplib::Response& response);
        // 处理发送消息的请求 -- 全量发送
        void handleSendMessage(const httplib::Request& request, httplib::Response& response);
        // 处理发送消息的请求 -- 增量发送
        void handleSendMessageStream(const httplib::Request& request, httplib::Response& response);
    private:
        // 服务器配置
        // 问：为什么要创建模型的配置信息？
        ChatServerConfig _config;  
        // 判断SDK是否存活
        std::atomic<bool> _isRunning = {false};  // 因为是多线程，应确保修改的时候是原子性的
        // 创建一个SDK对象，让其对下面的模型进行操作
        std::shared_ptr<ai_chat_sdk::ChatSDK> _sdk = nullptr;
        // 创建http服务器
        std::unique_ptr<httplib::Server> _chatServer = nullptr;
    };
} // end ai_chat_server
