#pragma once
#include "LLMProvider.h"

namespace ai_chat_sdk
{
    class ChatGPTProvider : public LLMProvider
    {
    public:
        bool InitModel(const std::map<std::string,std::string>& Model_Config) override;
        std::string GetModelName() override;
        std::string GetModelDesc() override;
        bool IsAvailable() override;
        std::string SendMessage(const std::vector<Message>& messages, const std::map<std::string,std::string>& request_param) override;
        std::string SendMessageStream(const std::vector<Message>& messages, 
                                  const std::map<std::string,std::string>& request_param,
                                  std::function<void(const std::string&,bool)> callback) override;
    };
}