#include "../include/ChatSDK.h"
#include "../include/DeepSeekProvider.h"
#include "../include/ChatGPTProvider.h"
#include "../include/GeminiProvider.h"
#include "../include/OllamaProvider.h"
#include <memory>
#include <unordered_set>



namespace ai_chat_sdk
{
    void ChatSDK::initModel(const std::vector<std::shared_ptr<Config>> configs)
    {
        // 注册所有模型
        registerAllModels(configs);
        // 初始化所有模型
        initAllModels(configs);
    }

    // 注册所有的模型信息
    // 问：这里的vector存的配置信息都是ollama吗
    void ChatSDK::registerAllModels(const std::vector<std::shared_ptr<Config>> configs)
    {
        // deepseek
        // 问：判断模型是否存活，是来判断模型是否被注册了，但是并没有初始化？
        if(!_llmManager.isModelAvailable("deepseek-flash"))
        {
            // 创建模型对象
            std::unique_ptr<DeepSeekProvider> deepSeekProvider = std::make_unique<DeepSeekProvider>();
            _llmManager.registerModel("deepseek-flash", std::move(deepSeekProvider));
            INFO("deepseek-chat provider registered successed");
        }
        // chatgpt
        if(!_llmManager.isModelAvailable("gpt-4o-mini"))
        {
            // 创建模型对象
            std::unique_ptr<ChatGPTProvider> chatGPTProvider = std::make_unique<ChatGPTProvider>();
            _llmManager.registerModel("gpt-4o-mini", std::move(chatGPTProvider));
            INFO("gpt-4o-mini-chat provider registered successed");
        }
        // gemini
        if(!_llmManager.isModelAvailable("gemini"))
        {
            // 创建模型对象
            std::unique_ptr<GeminiProvider> geminiProvider = std::make_unique<GeminiProvider>();
            _llmManager.registerModel("gemini", std::move(geminiProvider));
            INFO("gemini-chat provider registered successed");
        }
        // ollama ：可能存在多个模型，需要遍历configs一一注册
        // 1.创建一个set存放模型名，防止用户传入重复的模型名
        std::unordered_set<std::string> modelNames;
        // 2.遍历所有的配置信息，然后进行添加
        for(auto& config : configs)
        {
            // 模型传进来的配置参数可能不是ollama的,需要转换一下来确认
            auto ollamaConfig = std::dynamic_pointer_cast<OllamaConfig>(config);
            if(ollamaConfig)
            {
                std::string modelName = ollamaConfig->_modelName;
                if(modelNames.find(modelName) == modelNames.end())
                    // 将模型添加到set中
                    modelNames.insert(modelName);
                
                // 注册模型
                if(!_llmManager.isModelAvailable(modelName))
                {
                    // 创建模型对象
                    _llmManager.registerModel(modelName, std::make_unique<OllamaProvider>());
                    INFO("ollama-chat provider registered successed");
                }
            } 
        }
    }

    // 初始化所有的模型
    void ChatSDK::initAllModels(const std::vector<std::shared_ptr<Config>> configs)
    {
        for(auto& config : configs)
        {
            // 初始化api模型
            if(std::dynamic_pointer_cast<ApiConfig>(config))
            {
                // 判断传进来的模型是否是我们支持的模型
                if(config->_mode_name == "deepseek-falsh" ||
                   config->_mode_name == "gpt-4o-mini" ||
                   config->_mode_name == "gemini")
                {
                    // 注册模型
                }
            }
            else if(std::dynamic_pointer_cast<OllamaConfig>(config))
            {
                // 注册模型
            }
            else
            {
                ERROR("initAllModels: model is not supported!");
            }
        }
    }

    // 初始化api模型
    void ChatSDK::initApiModels(const std::shared_ptr<ApiConfig> config)
    {
        // 判断配置信息是否合法
        if(!config->_api_key.empty())
        {
            ERROR("initApiModels: api_key is empty!");
        }

        // 注册模型
        std::map<std::string,std::string> modelParams;
        modelParams["apikey"] = config->_api_key;
        _llmManager.initModel(config->_mode_name, modelParams);

        // 将模型信息和配置信息添加到_modelInfo中
        _modelConfig[config->_mode_name] = config;
    }

}// end ai_chat_sdk