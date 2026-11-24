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

        _isInit = true;
    }

    // 注册所有的模型信息
    // 问：这里的vector存的配置信息都是ollama吗
    void ChatSDK::registerAllModels(const std::vector<std::shared_ptr<Config>> configs)
    {
        // deepseek
        // 问：判断模型是否存活，是来判断模型是否被注册了，但是并没有初始化吗？
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
        if(!_llmManager.isModelAvailable("gemini-3.5-flash"))
        {
            // 创建模型对象
            std::unique_ptr<GeminiProvider> geminiProvider = std::make_unique<GeminiProvider>();
            _llmManager.registerModel("gemini-3.5-flash", std::move(geminiProvider));
            INFO("gemini-3.5-flash-chat provider registered successed");
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
                std::string modelName = ollamaConfig->_mode_name;
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
            if(auto apiConfig = std::dynamic_pointer_cast<ApiConfig>(config))
            {
                // 判断传进来的模型是否是我们支持的模型
                // 注意：这里的名字必须与 registerAllModels 中的注册名完全一致，
                // 否则 initModel 找不到对应的 provider
                if(config->_mode_name == "deepseek-flash" ||
                   config->_mode_name == "gpt-4o-mini" ||
                   config->_mode_name == "gemini-3.5-flash")
                {
                    // 注册模型
                    if(!initApiModels(apiConfig))
                    {
                        ERROR("initAllModels: init api model failed!");
                        continue;
                    }
                }
            }
            else if(auto ollamaConfig = std::dynamic_pointer_cast<OllamaConfig>(config))
            {
                // 注册模型
                if(!initOllamaModels(ollamaConfig))
                {
                    ERROR("initAllModels: init ollama model failed!");
                    continue;
                }
            }
            else
            {
                ERROR("initAllModels: model is not supported!");
            }
        }
    }

    // 初始化api模型
    bool ChatSDK::initApiModels(const std::shared_ptr<ApiConfig> config)
    {
        // 判断配置信息是否合法
        if(config->_api_key.empty())
        {
            ERROR("initApiModels: api_key is empty!");
            return false;
        }

        // 注册模型
        std::map<std::string,std::string> modelParams;
        modelParams["apikey"] = config->_api_key;
        _llmManager.initModel(config->_mode_name, modelParams);

        // 将模型信息和配置信息添加到_modelInfo中
        _modelConfig[config->_mode_name] = config;
        return true;
    }

    // 初始化ollama模型
    bool ChatSDK::initOllamaModels(const std::shared_ptr<OllamaConfig> config)
    {
        // 判断配置信息是否合法
        if(config->_endpoint.empty())
        {
            ERROR("initOllamaModels: endpoint is empty!");
            return false;
        }
        
        // 初始化模型
        std::map<std::string,std::string> modelParams;
        modelParams["base_url"] = config->_endpoint;
        modelParams["model_name"] = config->_mode_name;
        modelParams["model_desc"] = config->_modelDesc;
        _llmManager.initModel(config->_mode_name, modelParams);
        return true;
    }

    // 创建一个会话
    std::string ChatSDK::createSession(const std::string& modelName)
    {
        // 判断ChatSDK是否初始化
        if(!_isInit)
        {
            ERROR("createSession: ChatSDK is not initialized!");
            return "";
        }

        // 创建会话
        std::string sessionId = _sessionManager.initSession(modelName);
        if(sessionId.empty())
        {
            ERROR("ChatSDK::createSession: create session failed");
            return "";
        }

        INFO("ChatSDK::createSession: create session success, sessionId = {}", sessionId);
        return sessionId;
    }  

    // 获取指定会话
    std::shared_ptr<Session> ChatSDK::getSessionInfo(const std::string& sessionId)
    {
        // 判断ChatSDK是否初始化
        if(!_isInit)
        {
            ERROR("getSessionInfo: ChatSDK is not initialized!");
            return nullptr;
        }

        // 获取会话信息
        auto session = _sessionManager.getSession(sessionId);
        if(!session)
        {
            ERROR("getSessionInfo: session {} not found!", sessionId);
            return nullptr;
        }
        return session;
    }

    // 删除指定会话
    bool ChatSDK::deleteSession(const std::string& sessionId)
    {
        // 判断ChatSDK是否初始化
        if(!_isInit)
        {
            ERROR("deleteSession: ChatSDK is not initialized!");
            return false;
        }

        // 删除会话
        bool isDelete = _sessionManager.deleteSession(sessionId);
        if(isDelete)
        {
            INFO("ChatSDK::deleteSession: delete session success, sessionId = {}", sessionId);
        }
        else
        {
            ERROR("ChatSDK::deleteSession: delete session failed, sessionId = {}", sessionId);
        }
        return isDelete;
    }

    // 获取所有会话Id
    std::vector<std::string> ChatSDK::getSessionIds()
    {
        // 判断ChatSDK是否初始化
        if(!_isInit)
        {
            ERROR("getSessionIds: ChatSDK is not initialized!");
            return {};
        }

        // 获取所有会话id
        return _sessionManager.getSessionLists();
    }

    // 获取可用模型信息
    std::vector<ModelInfo> ChatSDK::getModelInfo()
    {
        // 判断ChatSDK是否初始化
        if(!_isInit)
        {
            ERROR("getModelInfo: ChatSDK is not initialized!");
            return {};
        }

        // 获取所有模型信息
        return _llmManager.getAvailableModels();
    }

    // 给模型发信息 -- 全量
    std::string ChatSDK::sendMessage(const std::string& sessionId, const std::string& message)
    {
        // 判断ChatSDK是否初始化
        if(!_isInit)
        {
            ERROR("sendMessage: ChatSDK is not initialized!");
            return "";
        }

        auto session = _sessionManager.getSession(sessionId);
        if(!session)
        {
            ERROR("sendMessage: session {} not found", sessionId);
            return "";
        }

        Message new_message("user",message);
        if(_sessionManager.addMessage(sessionId,new_message))
        {
            // 获取所有的消息并发给模型
            auto history_messages = _sessionManager.getHistoryMessage(sessionId);

            // 构建请求参数
            auto it = _modelConfig.find(session->_mode_name);
            std::map<std::string,std::string> params;
            if(it == _modelConfig.end())
            {
                ERROR("sendMessage: model {} not found", session->_mode_name);
                return "";
            }
            // 注意：map<string,string> 需要字符串，必须 std::to_string 转换；
            // 且 Provider 读取的 key 是 max_tokens（带 s）
            params["temperature"] = std::to_string(it->second->_temperature);
            params["max_tokens"] = std::to_string(it->second->_max_token);
            // 给模型发信息
            std::string reply_message = _llmManager.sendMessage(session->_mode_name, history_messages,params);
            if(reply_message.empty())
            {
                ERROR("sendMessage: send message to model failed, sessionId = {}", sessionId);
                return "";
            }
            
            // 添加助手消息并更新会话时间
            Message assistantMessage("assistant", reply_message);
            _sessionManager.addMessage(sessionId, assistantMessage);
            _sessionManager.updateSessionTimestamp(sessionId);
            INFO("ChatSDK::sendMessage: send message to model {} successed", session->_mode_name);
            return reply_message;
        }
        else
        {
            ERROR("sendMessage: add message to session failed, sessionId = {}", sessionId);
            return "";
        }
    }

    // 给模型发信息 -- 流式
    std::string ChatSDK::sendMessageIncrement(const std::string &sessionId, const std::string &message, std::function<void (std::string, bool)> callback)
    {
        // 判断ChatSDK是否初始化
        if(!_isInit)
        {
            ERROR("sendMessage: ChatSDK is not initialized!");
            return "";
        }

        // 通过会话id获取相应会话信息,是为了获取模型名称
        auto session = _sessionManager.getSession(sessionId);
        if(!session)
        {
            ERROR("sendMessage: session {} not found", sessionId);
            return "";
        }

        // 将user的信息添加到信息列表中
        Message user_message("user",message);
        _sessionManager.addMessage(sessionId,user_message);
        // 获取历史会话消息
        auto history_messages = _sessionManager.getHistoryMessage(sessionId);

        // 在配置列表中找相应模型
        auto it = _modelConfig.find(session->_mode_name);
        if(it == _modelConfig.end())
        {
            ERROR("sendMessageIncrement: model {} not found", session->_mode_name);
            return "";
        }
        // 构建配置信息
        std::map<std::string,std::string> params;
        params["temperature"] = std::to_string(it->second->_temperature);
        params["max_tokens"] = std::to_string(it->second->_max_token);

        // 发送消息
        std::string reply_message = _llmManager.sendMessageStream(session->_mode_name,history_messages,params,callback);
        if(reply_message.empty())
        {
            ERROR("sendMessageIncrement: send message to model failed, sessionId = {}", sessionId);
            return "";
        }

        // 添加助手消息并更新会话时间
        Message assistantMessage("assistant", reply_message);
        _sessionManager.addMessage(sessionId, assistantMessage);
        _sessionManager.updateSessionTimestamp(sessionId);
        INFO("ChatSDK::sendMessageIncrement: send message to model {} successed", session->_mode_name);
        return reply_message;

    }
}// end ai_chat_sdk