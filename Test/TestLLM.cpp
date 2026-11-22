#include "gtest/gtest.h"
#include <memory>
#include "../sdk/include/util/log.h"
#include "../sdk/include/ChatGPTProvider.h"
#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/GeminiProvider.h"
#include "../sdk/include/OllamaProvider.h"
#include "../sdk/include/common.h"
#include "../sdk/include/ChatSDK.h"

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

#endif

TEST(ChatSDKTest,SendMessage)
{
    // 构建模型的配置信息
    // deepseek 模型的配置信息
    std::shared_ptr<ai_chat_sdk::ApiConfig> deepseek = std::make_shared<ai_chat_sdk::ApiConfig>();
    ASSERT_TRUE(deepseek);
    deepseek->_mode_name = "deepseek-flash";
    deepseek->_max_token = 2048;
    deepseek->_temperature = 0.7;
    const char* deepseekKey = getenv("DeepSeek_apikey");
    ASSERT_NE(deepseekKey, nullptr) << "环境变量 DeepSeek_apikey 未设置";
    deepseek->_api_key = deepseekKey;

    // chatgpt 模型的配置信息
    std::shared_ptr<ai_chat_sdk::ApiConfig> chatgpt = std::make_shared<ai_chat_sdk::ApiConfig>();
    ASSERT_TRUE(chatgpt);
    chatgpt->_mode_name = "gpt-4o-mini";
    chatgpt->_max_token = 2048;
    chatgpt->_temperature = 0.7;
    const char* chatgptKey = getenv("ChatGPT_apikey");
    ASSERT_NE(chatgptKey, nullptr) << "环境变量 ChatGpt_apikey 未设置";
    chatgpt->_api_key = chatgptKey;

    // gemini 模型的配置信息
    std::shared_ptr<ai_chat_sdk::ApiConfig> gemini = std::make_shared<ai_chat_sdk::ApiConfig>();
    ASSERT_TRUE(gemini);
    gemini->_mode_name = "gemini-3.5-flash";
    gemini->_max_token = 2048;
    gemini->_temperature = 0.7;
    const char* geminiKey = getenv("Gemini_apikey");
    ASSERT_NE(geminiKey, nullptr) << "环境变量 Gemini_apikey 未设置";
    gemini->_api_key = geminiKey;

    // ollama 模型的配置信息
    std::shared_ptr<ai_chat_sdk::OllamaConfig> ollama = std::make_shared<ai_chat_sdk::OllamaConfig>();
    ASSERT_TRUE(ollama);
    ollama->_mode_name = "deepseek-r1:1.5b";
    ollama->_modelDesc = "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理";
    ollama->_endpoint = "http://localhost:11434";
    ollama->_max_token = 2048;
    ollama->_temperature = 0.7;

    // 构造vector存放Config
    std::vector<std::shared_ptr<ai_chat_sdk::Config>> Configs = {
        deepseek,chatgpt,gemini,ollama
    };

    // 初始化模型
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    sdk->initModel(Configs);
    ASSERT_TRUE(sdk);

    // 创建会话
    std::string sessionId = sdk->createSession(deepseek->_mode_name);
    ASSERT_TRUE(!sessionId.empty());

    // 发送第一条信息
    std::string message;
    std::cout << ">>> ";
    std::getline(std::cin, message); 
    auto response = sdk->sendMessage(sessionId, message);
    ASSERT_FALSE(response.empty());

    // 发送第二条消息
    std::cout << ">>> ";
    std::getline(std::cin, message); 
    response = sdk->sendMessage(sessionId, message);
    ASSERT_FALSE(response.empty());

    // 打印历史消息
    auto historyMessages = sdk->_sessionManager.getHistoryMessage(sessionId);
    for(const auto& msg : historyMessages)
    {
        INFO("message role : {} , message : {}",msg._role, msg._content);
    }
    
}

int main(int argc,char* argv[])
{
    Jammy::Logger::LoggerInit("testLLM","stdout",spdlog::level::info);
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}