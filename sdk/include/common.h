#pragma once
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace ai_chat_sdk
{
    // 消息管理
    struct Message
    {
        string _id;  
        string _role;       
        string _content;    
        time_t _timestamp;  // 发送消息时的时间

        // 问：为什么这里没有初始化_messageid？
        Message(string role, string content)
        : _role(role), _content(content), _timestamp(time(nullptr))
        {
        }
    };

    // 会话管理
    struct Session
    {
        string _id;
        string _mode_name;  // 模型名称
        vector<Message> _messages;  // 管理消息
        time_t _createAt;  // 创建会话的时间
        time_t _updateAt;  // 该会话最新一次消息的时间

        Session(string mode = "")
        :_mode_name(mode),_createAt(time(nullptr)),_updateAt(time(nullptr))
        {}
    };

    // 问：为什么不将api放在公共配置中
    // 公共配置
    struct Config
    {
        string _mode_name;
        double _temperature = 0.7;  // 模型设置的温度，控制模型回复的准确度
        int _max_token = 2048;  // 输入的最大token数

        Config(string mode = "")
        :_mode_name(mode)
        {}
    };

    // 问：struct也能继承基类？
    // 连接云端大模型
    struct ApiConfig : public Config
    {
        string _api_key;
    };

    // LLM的描述
    struct ModleInfo
    {
        string _mode_name;
        string _desc;
        string _provider;  // 模型提供者
        string _endpoint;  // 服务器的地址
        bool _IsAvaiable;

        ModleInfo(string mode = "", string desc = "",string provider = "", string endpoint = "")
        :_mode_name(mode),_desc(desc),_provider(provider),_endpoint(endpoint),_IsAvaiable(false)
        {}
    };
}
