#pragma once
#include "./util/log.h"
#include "common.h"
#include "LLMManager.h"
#include "SessionManager.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

// 该功能是将LLMManager和SessionManager封装起来，形成一个完整的聊天SDK
namespace ai_chat_sdk
{
    using namespace Jammy;
    class ChatSDK
    {
    public:
        // 初始化模型 -- 可能含有多个模型配置
        // 问：为什么这里用vector?   为什么要用shared_ptr?  
        void initModel(const std::vector<std::shared_ptr<Config>> configs);
        // 创建一个会话
        std::string createSession(const std::string& modelName);
        // 删除指定会话
        bool deleteSession(const std::string& sessionId);
        // 获取指定会话信息
        std::shared_ptr<Session> getSessionInfo(const std::string& sessionId);
        // 获取所有会话id
        std::vector<std::string> getSessionIds();
        // 获取可用模型信息
        std::vector<ModelInfo> getModelInfo();
        // 给模型发信息 -- 全量
        // 问：为什么这里需要会话id，不需要模型名？
        std::string sendMessage(const std::string& sessionId, const std::string& message);
        // 给模型发信息 -- 增量
        std::string sendMessageIncrement(const std::string& sessionId,
                                        const std::string& message,
                                        std::function<void(std::string, bool)> callback);
    private:
        // 注册所有的模型信息
        void registerAllModels(const std::vector<std::shared_ptr<Config>> configs);
        // 初始化所有的模型
        void initAllModels(const std::vector<std::shared_ptr<Config>> configs);
        // 初始化api模型

        bool initApiModels(const std::shared_ptr<ApiConfig> config);
        // 初始化ollama模型
        bool initOllamaModels(const std::shared_ptr<OllamaConfig> config);
    private:
        bool _isInit = false;
        // 问：为什么需要记录模型配置？直接存放模型名不就可以，用一个数组
        std::unordered_map<std::string, std::shared_ptr<Config>> _modelConfig;
        LLMManager _llmManager;
    public:
        SessionManager _sessionManager;   // 会话管理中就包含了数据管理
    };
}// end ai_chat_sdk