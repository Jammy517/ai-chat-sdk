#pragma once
#include <string>
#include <map>
#include <vector>
#include <functional>
#include "common.h"

namespace ai_chat_sdk
{
    class LLMProvider
    {
    public:
        // 问：为什么要用map初始化模型？map中存什么
        virtual bool InitModel(const std::map<std::string,std::string>& Model_Config) = 0;
        virtual std::string GetModelName() = 0;
        virtual std::string GetModelDesc() = 0;
        virtual bool IsAvailable() = 0;
        // 问：这里的vector的作用？map的作用？request_param中存什么？
        // 给大模型发送消息 -- 全量发送
        virtual std::string SendMessage(const std::vector<Message>& messages, const std::map<std::string,std::string>& request_param) = 0;
        // 问：为什么要用function包装器回调函数
        // 给大模型发送消息 -- 增量发送
        virtual std::string SendMessageStream(const std::vector<Message>& messages, 
                                  const std::map<std::string,std::string>& request_param,
                                  std::function<void(const std::string&,bool)> callback) = 0;
    protected:
        bool _isAvailable = false;   // 模型是否存活
        std::string _apikey;
        std::string _base_url;  // 模型地址 
    };
}