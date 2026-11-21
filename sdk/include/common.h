#pragma once
#include <iostream>
#include <string>
#include <vector>


namespace ai_chat_sdk
{
    // 消息管理
    struct Message
    {
        std::string _id;  
        std::string _role;       
        std::string _content;    
        time_t _timestamp;  // 发送消息时的时间

        // 问：为什么这里没有初始化_messageid？
        Message(std::string role, std::string content)
        : _role(role), _content(content), _timestamp(time(nullptr))
        {
        }
    };

    // 会话管理
    struct Session
    {
        std::string _id;
        std::string _mode_name;  // 会话使用的模型名称
        std::vector<Message> _messages;  // 管理消息
        time_t _createAt;  // 创建会话的时间
        time_t _updateAt;  // 该会话最新一次消息的时间

        Session(std::string mode = "")
        :_mode_name(mode),_createAt(time(nullptr)),_updateAt(time(nullptr))
        {}
    };

    // 问：为什么不将api放在公共配置中
    // 公共配置
    struct Config
    {
        std::string _mode_name;
        double _temperature = 0.7;  // 模型设置的温度，控制模型回复的准确度
        int _max_token = 2048;  // 输入的最大token数

        Config(std::string mode = "")
        :_mode_name(mode)
        {}
    };

    // 问：struct也能继承基类？
    // 连接云端大模型
    struct ApiConfig : public Config
    {
        std::string _api_key;
    };

    // 接入本地大模型
    struct OllamaConfig : public Config
    {
        // 问：这里需要添加模型名字和模型描述吗
        std::string _modelDesc;
        std::string _modelName;
        std::string _endpoint;
    };

    // LLM的描述
    struct ModelInfo
    {
        std::string _mode_name;
        std::string _desc;
        std::string _provider;  // 模型提供者
        std::string _endpoint;  // 服务器的地址
        bool _isAvailable;  // 是否在线

        ModelInfo(std::string mode = "", std::string desc = "",std::string provider = "", std::string endpoint = "")
        :_mode_name(mode),_desc(desc),_provider(provider),_endpoint(endpoint),_isAvailable(false)
        {}
    };
} // end ai_chat_sdk
