#include "ChatServer.h"

namespace ai_chat_server
{
    using namespace Jammy;
    ChatServer::ChatServer(const ChatServerConfig& config)
    {
        // 初始化SDK
        _sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
        std::shared_ptr<ai_chat_sdk::ApiConfig> deepseek = std::make_shared<ai_chat_sdk::ApiConfig>();
        deepseek->_api_key = config.deepseek_apikey;
        deepseek->_max_token = config._max_token;
        deepseek->_mode_name = "deepseek-flash";
        deepseek->_temperature = config._temperature;

        std::shared_ptr<ai_chat_sdk::ApiConfig> chatgpt = std::make_shared<ai_chat_sdk::ApiConfig>();
        chatgpt->_api_key = config.chatgpt_apikey;
        chatgpt->_max_token = config._max_token;
        chatgpt->_mode_name = "gpt-4o-mini";
        chatgpt->_temperature = config._temperature;

        std::shared_ptr<ai_chat_sdk::ApiConfig> gemini = std::make_shared<ai_chat_sdk::ApiConfig>();
        gemini->_api_key = config.gemini_apikey;
        gemini->_max_token = config._max_token;
        gemini->_mode_name = "gemini-3.5-flash";
        gemini->_temperature = config._temperature;

        std::shared_ptr<ai_chat_sdk::OllamaConfig> ollama = std::make_shared<ai_chat_sdk::OllamaConfig>();
        ollama->_endpoint = config._ollama_endpoint;
        ollama->_max_token = config._max_token;
        ollama->_mode_name = config._ollama_model;
        ollama->_modelDesc = config._ollama_desc;
        ollama->_temperature = config._temperature;

        // 将配置信息存放在数组中，方便后面传递
        std::vector<std::shared_ptr<ai_chat_sdk::Config>> Configs = {
            deepseek,chatgpt,gemini,ollama
        };
        INFO("ChatServer: init model success");

        // 初始化模型
        _sdk->initModel(Configs);

        // 初始服务器
        _chatServer = std::make_unique<httplib::Server>();
        if(!_chatServer)
        {
            ERROR("ChatServer init Failed!!!");
            return;
        }
    }

    // 启动服务器
    bool ChatServer::start()
    {
        // 判断服务器是否处于运行状态
        if(_isRunning.load())
        {
            ERROR("ChatServer is running;");
            return false;
        }

        // 创建一个线程，让这个线程去监听客户端的请求
        std::thread thread([this](){
            // 问：这里的_config不是没有进行初始化吗？我的构造函数并没有初始化_config？
            INFO("ChatServer start on {} :{}",_config._ip,_config._port);
            _chatServer->listen(_config._ip, _config._port);
        });

        // 启动线程
        thread.detach();  // 目的是为了让主线程与子线程分离
        _isRunning.store(true);
        INFO("ChatServer start success!!!");
        return true;
    }

    // 停止服务器
    bool ChatServer::stop()
    {
        // 判断服务器是否处于运行状态
        if(!_isRunning.load())
        {
            ERROR("ChatServer is not running;");
            return false;
        }
        // 停止服务器监听
        _chatServer->stop();  
        // 问：关闭服务器线程如何回收  

        _isRunning.store(false);
        INFO("ChatServer stop success!!!");
        return true;
    }

    // 判断服务器是否存活
    bool ChatServer::isAvailable() const
    {
        return _isRunning.load();
    }
} // end ai_chat_server