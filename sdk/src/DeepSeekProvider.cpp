#pragma once
#include "../include/DeepSeekProvider.h"
#include "../include/util/log.h"

namespace ai_chat_sdk
{
    using namespace Jammy;

    class deepseek : public LLMProvider
    {
    public:
        // map存放的是大模型的配置信息的变量和配置信息对应的具体内容
        bool InitModel(const std::map<std::string,std::string>& Model_Config) override
        {
            // 查看api_key
            auto it = Model_Config.find("api_key");
            if(it == Model_Config.end())
            {
                ERROR("DeepSeekProvider: api_key is empty!");
                return false;
            }
            else
            {
                _apikey = it->second;
                INFO("DeepSeekProvider: api_key set success!");
            }

            // 查看base_url
            it = Model_Config.find("base_url");
            if(it == Model_Config.end())
            {
                ERROR("DeepSeekProvider: base_url is empty!");
                return false;
            }
            else
            {
                _base_url = it->second;
                INFO("DeepSeekProvider: base_url set success!");
            }

            INFO("init success!!!");
            _isAvailable = true;
            return true;
        }
        std::string GetModelName() override
        {
            return "deepseek-flash";
        }
        std::string GetModelDesc() override
        {
            return "⼀款实⽤性强、中⽂优化的通⽤对话助⼿，适合⽇常问答与创作";
        }
        bool IsAvailable() override
        {
            return _isAvailable;
        }
        std::string SendMessage(const std::vector<Message>& messages, const std::map<std::string,std::string>& request_param) override
        {

        }
        std::string SendMessageStream(const std::vector<Message>& messages, 
                                  const std::map<std::string,std::string>& request_param,
                                  std::function<void(const std::string&,bool)> callback) override
        {
            
        }
    };
}