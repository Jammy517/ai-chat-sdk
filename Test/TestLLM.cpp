#include "gtest/gtest.h"
#include "../sdk/include/util/log.h"
#include "../sdk/include/DeepSeekProvider.h"

using namespace Jammy;

// 问：这两个参数是什么意思
TEST(DeepSeekProviderTest,SendMessageDeepSeek)
{
    // 1.初始化大模型
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider);
    // 设置初始化的参数变量
    std::map<std::string,std::string> config;
    config["apikey"] = getenv("deepseek_apikey");
    ASSERT_FALSE(config["apikey"].empty());
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

int main(int argc,char* argv[])
{
    Jammy::Logger::LoggerInit("testLLM","stdout",spdlog::level::info);
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}