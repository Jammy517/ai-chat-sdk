#include "gtest/gtest.h"
#include "../sdk/include/util/log.h"
#include "../sdk/include/DeepSeekProvider.h"

// 问：这两个参数是什么意思
TEST(DeepSeekProviderTest,SendMessageDeepSeek)
{
    // 1.初始化大模型
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider);
    // 设置初始化的参数变量
    std::map<std::string,std::string> config;
    // config["api_key"] = "sk-5b164a22b84043d0893498acdae41b25";
    config["api_key"] = getenv("deepseek_apikey");
    ASSERT_FALSE(config["api_key"].empty());
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
    auto response = provider->SendMessage(messages,request_param);
    ASSERT_FALSE(response.empty());
    
}

int main(int argc,char* argv[])
{
    Jammy::Logger::LoggerInit("testLLM","stdout",spdlog::level::debug);
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}