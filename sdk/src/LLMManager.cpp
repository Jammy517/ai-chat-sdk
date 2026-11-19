#include "../include/LLMManager.h"
#include "../include/util/log.h"

namespace ai_chat_sdk
{
    using namespace Jammy;
    bool LLMManager::registerModel(const std::string& model_name,std::unique_ptr<LLMProvider> provider)
    {
        if(!provider)
        {
            ERROR("provider is null, register fail");
            return false;
        }

        // 防止重复注册：若已存在同名 provider，拒绝覆盖
        if(_providers.find(model_name) != _providers.end())
        {
            ERROR("model_name '{}' already registered, skip", model_name);
            return false;
        }

        // 添加模型提供者信息
        _providers[model_name] = std::move(provider);
        // 添加模型配置信息
        _modelInfo[model_name] = ModelInfo(model_name);

        return true;
    }

    bool LLMManager::initModel(const std::string& model_name, const std::map<std::string,std::string>& Model_Config)
    {
        // 判断模型是否被注册
        auto it = _providers.find(model_name);
        if(it == _providers.end())
        {
            ERROR("model is not be providered, init fail");
            return false;
        }

        bool isInit = it->second->InitModel(Model_Config);
        if(isInit)
        {
            INFO("init model success, modelName = {}", model_name);
            _modelInfo[model_name]._desc = it->second->GetModelDesc();
            _modelInfo[model_name]._isAvailable = true;
        }
        else{
            ERROR("init {} model fail",model_name);
            return false;
        }
        return true;
    }

    std::vector<ModelInfo> LLMManager::getAvailableModels()
    {
        std::vector<ModelInfo> models;
        for(auto& model : _modelInfo)
        {
            if(model.second._isAvailable)
                models.push_back(model.second);
        }
        return models;
    }

    bool LLMManager::isModelAvailable(const std::string& model_name)
    {
        // 确认模型是否存在
        auto it = _modelInfo.find(model_name);
        if(it == _modelInfo.end())
        {
            ERROR("model is not exist");
            return false;
        }
        // 判断模型是否存活
        return _modelInfo[model_name]._isAvailable;
    }

    std::string LLMManager::sendMessage(const std::string& model_name, 
                                        const std::vector<Message>& messages, 
                                        const std::map<std::string,std::string>& request_param)
    {
        // 判断模型是否存在
        auto it = _providers.find(model_name);
        if(it == _providers.end())
        {
            ERROR("model is not exist");
            return "";
        }

        // 判断模型是否存活
        // if(!it->second->IsAvailable())
        // {
        //     ERROR("model is not available");
        //     return "";
        // }

        if(!isModelAvailable(model_name))
        {
            ERROR("model is not available");
            return "";
        }

        return it->second->SendMessage(messages,request_param);
    }

    std::string LLMManager::sendMessageStream(const std::string& model_name,const std::vector<Message>& messages, 
                                   const std::map<std::string,std::string>& request_param,
                                   std::function<void(const std::string&,bool)> callback)
    {
        // 判断模型是否存在
        auto it = _providers.find(model_name);
        if(it == _providers.end())
        {
            ERROR("model is not exist");
            return "";
        }

        // 判断模型是否存活
        if(!it->second->IsAvailable())
        {
            ERROR("model is not available");
            return "";
        }

        return it->second->SendMessageStream(messages,request_param,callback);
    }
} // end ai_chat_sdk
