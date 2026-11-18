#pragma once
#include "LLMProvider.h"

namespace ai_chat_sdk
{
    class OllamaProvider : public LLMProvider
    {
    public:
        // 问：为什么要用map初始化模型？map中存什么
        bool InitModel(const std::map<std::string,std::string>& Model_Config) override;
        std::string GetModelName() override;
        std::string GetModelDesc() override;
        bool IsAvailable() override;
        // 问：这里的vector的作用？map的作用？request_param中存什么？
        // 给大模型发送消息 -- 全量发送
        std::string SendMessage(const std::vector<Message>& messages, const std::map<std::string,std::string>& request_param) override;
        // 问：为什么要用function包装器回调函数
        // 给大模型发送消息 -- 增量发送
        std::string SendMessageStream(const std::vector<Message>& messages, 
                                  const std::map<std::string,std::string>& request_param,
                                  std::function<void(const std::string&,bool)> callback) override;
    private:
        std::string _modelName;
        std::string _modelDesc;
    };
}