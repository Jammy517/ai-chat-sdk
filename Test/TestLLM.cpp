#include "gtest/gtest.h"
#include "../sdk/include/util/log.h"
#include "../sdk/include/ChatGPTProvider.h"
#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/GeminiProvider.h"
#include "../sdk/include/OllamaProvider.h"

using namespace Jammy;

#if 0

// 问：这两个参数是什么意思
TEST(DeepSeekProviderTest,SendMessageChatGPT)
{
    // 1.初始化大模型
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider);
    // 设置初始化的参数变量
    std::map<std::string,std::string> config;
    const char* apikey = getenv("DeepSeek_apikey");
    ASSERT_NE(apikey, nullptr) << "环境变量 DeepSeek_apikey 未设置";
    config["apikey"] = apikey;
    config["base_url"] = "https://api.deepseek.com";
    provider->InitModel(config);
    ASSERT_TRUE(provider->IsAvailable());

    // 2.设置SendMessahe中的参数
    std::map<std::string,std::string> request_param = {
        {"temperature","0.7"},
        {"max_tokens","2048"}
    };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back(ai_chat_sdk::Message("user","你是谁?"));
    // 3.设置回调函数
    auto callback = [](const std::string& content,bool isFinish)
    {
        INFO("chunk : {}", content);
        if(isFinish)
        {
            INFO("DONE");
        }
    };
    
    // auto response = provider->SendMessageStream(messages,request_param, callback);
    auto response = provider->SendMessage(messages,request_param);
    ASSERT_FALSE(response.empty());
    INFO("fulldata : {}", response);
    
}

TEST(GeminiProviderTest,SendMessageChatGPT)
{
    // 1.初始化大模型
    auto provider = std::make_shared<ai_chat_sdk::GeminiProvider>();
    ASSERT_TRUE(provider);
    // 设置初始化的参数变量
    std::map<std::string,std::string> config;
    const char* apikey = getenv("Gemini_apikey");
    ASSERT_NE(apikey, nullptr) << "环境变量 gemini_apikey 未设置";
    config["apikey"] = apikey;
    ASSERT_FALSE(config["apikey"].empty());
    config["base_url"] = "https://generativelanguage.googleapis.com";
    provider->InitModel(config);
    ASSERT_TRUE(provider->IsAvailable());

    // 2.设置SendMessahe中的参数
    std::map<std::string,std::string> request_param = {
        {"temperature","0.7"},
        {"max_tokens","2048"}
    };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back(ai_chat_sdk::Message("user","你是谁?"));
    // 3.设置回调函数
    auto callback = [](const std::string& content,bool isFinish)
    {
        INFO("chunk : {}", content);
        if(isFinish)
        {
            INFO("DONE");
        }
    };
    
    auto response = provider->SendMessageStream(messages,request_param, callback);
    // auto response = provider->SendMessage(messages,request_param);
    ASSERT_FALSE(response.empty());
    INFO("fulldata : {}", response);
    
}

TEST(ChatGPTProviderTest,SendMessageChatGPT)
{
    // 1.初始化大模型
    auto provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
    ASSERT_TRUE(provider);
    // 设置初始化的参数变量
    std::map<std::string,std::string> config;
    config["apikey"] = getenv("chatgpt_apikey");
    ASSERT_FALSE(config["apikey"].empty());
    config["base_url"] = "https://api.openai.com";
    provider->InitModel(config);
    ASSERT_TRUE(provider->IsAvailable());

    // 2.设置SendMessahe中的参数
    std::map<std::string,std::string> request_param = {
        {"temperature","0.7"},
        {"max_tokens","2048"}
    };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back(ai_chat_sdk::Message("user","你是谁?"));
    // 3.设置回调函数
    auto callback = [](const std::string& content,bool isFinish)
    {
        INFO("chunk : {}", content);
        if(isFinish)
        {
            INFO("DONE");
        }
    };
    
    // auto response = provider->SendMessageStream(messages,request_param, callback);
    auto response = provider->SendMessage(messages,request_param);
    ASSERT_FALSE(response.empty());
    INFO("fulldata : {}", response);
    
}

#endif

TEST(OllamaProviderTest,SendMessageChatGPT)
{
    // 1.初始化大模型
    auto provider = std::make_shared<ai_chat_sdk::OllamaProvider>();
    ASSERT_TRUE(provider);
    // 设置初始化的参数变量
    std::map<std::string,std::string> config;
    config["model_name"] = "deepseek-r1:1.5b";
    config["model_desc"] = "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理";
    config["base_url"] = "http://localhost:11434";
    provider->InitModel(config);
    ASSERT_TRUE(provider->IsAvailable());

    // 2.设置SendMessahe中的参数
    std::map<std::string,std::string> request_param = {
        {"temperature","0.7"},
        {"max_tokens","2048"}
    };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back(ai_chat_sdk::Message("user","你是谁?"));
    // 3.设置回调函数
    auto callback = [](const std::string& content,bool isFinish)
    {
        INFO("chunk : {}", content);
        if(isFinish)
        {
            INFO("DONE");
        }
    };
    
    auto response = provider->SendMessageStream(messages,request_param, callback);
    // auto response = provider->SendMessage(messages,request_param);
    ASSERT_FALSE(response.empty());
    INFO("fulldata : {}", response);
    
}

int main(int argc,char* argv[])
{
    Jammy::Logger::LoggerInit("testLLM","stdout",spdlog::level::info);
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}