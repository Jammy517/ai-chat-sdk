#pragma once
#include <map>
#include <string>
#include <memory>
#include "LLMProvider.h"
#include "common.h"
#include <vector>
#include <functional>

namespace ai_chat_sdk
{
    class LLMManager
    {
    public:
        // 注册模型提供者
        // 问：为什么要提供注册模型这个函数
        bool registerModel(const std::string& model_name,std::unique_ptr<LLMProvider> provider);
        // 初始化指定模型
        bool initModel(const std::string& model_name,const std::map<std::string,std::string>& Model_Config);
        // 获取可用模型信息
        std::vector<ModelInfo> getAvailableModels();
        // 判断模型是否存活
        bool isModelAvailable(const std::string& model_name);
        // 发送信息
        std::string sendMessage(const std::string& model_name,const std::vector<Message>& messages, 
                                const std::map<std::string,std::string>& request_param);
        // 发送流信息
        std::string sendMessageStream(const std::string& model_name,const std::vector<Message>& messages, 
                                   const std::map<std::string,std::string>& request_param,
                                   std::function<void(const std::string&,bool)> callback);
    private:
        // key : 模型名字  value : 模型提供者
        std::map<std::string,std::unique_ptr<LLMProvider>> _providers;  // 存放模型提供者
        // key : 模型名字  value : 模型信息
        std::map<std::string,ModelInfo> _modelInfo;  // 存放模型信息
    };
} // end ai_chat_sdk