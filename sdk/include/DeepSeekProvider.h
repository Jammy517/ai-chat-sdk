#pragma once
#include "LLMProvider.h"

namespace ai_chat_sdk
{
    class deepseek : public LLMProvider
    {
    public:
        virtual bool InitModel(const std::map<std::string,std::string>& Model_Config) = 0;
        virtual std::string GetModelName() = 0;
        virtual std::string GetModelDesc() = 0;
        virtual bool IsAvailable() = 0;
        virtual std::string SendMessage(const std::vector<Message>& messages, const std::map<std::string,std::string>& request_param) = 0;
        virtual std::string SendMessageStream(const std::vector<Message>& messages, 
                                  const std::map<std::string,std::string>& request_param,
                                  std::function<void(const std::string&,bool)> callback) = 0;
    };
}