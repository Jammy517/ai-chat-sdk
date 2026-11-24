#include "ChatServer.h"
#include <ai_chat_sdk/util/log.h>
#include <cstdint>
#include <jsoncpp/json/forwards.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/reader.h>
#include <vector>

namespace ai_chat_server
{
    using namespace Jammy;
    ChatServer::ChatServer(const ChatServerConfig& config)
    : _config(config)
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

        std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
            deepseek, chatgpt, gemini, ollama
        };
        _sdk->initModel(modelConfigs);
        INFO("ChatServer: init model success");

        // 初始服务器
        _chatServer = std::make_unique<httplib::Server>();
        if(!_chatServer)
        {
            ERROR("ChatServer init Failed!!!");
            return;
        }

        // 注册路由
        setRoutes();
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

        // 设置静态资源的路径
        // 前端页面相关的所有文件都放在www目录下  注意：将来前端页面名称命名为index.html
        // 当用户在浏览器中输入：http://ip:port/index.html    http://ip:port也能访问index.html页面
        // 在httplib中，默认情况下，如果请求路径中只有ip和端口，httplib默认会使用index.html文件
        _chatServer->set_mount_point("/", "./www");

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

    std::string ChatServer::buildResponse(const std::string& message, bool success)
    {
        Json::Value responseJson;
        responseJson["success"] = success;
        responseJson["message"] = message;

        // 序列化
        Json::StreamWriterBuilder write;
        return Json::writeString(write, responseJson);
    }

     // 处理获取会话列表的请求
    void ChatServer::handleGetSessionLists(const httplib::Request& request, httplib::Response& response)
    {
        // 因为发起请求直接就在服务器内部进行查找，不需要解析客户端传来的请求体，直接构建应答返回
        std::vector<std::string> sessionIds = _sdk->getSessionIds();
        // 循环遍历每个会话，然后构建完整的会话进行返回
        // 构建json对象存放所有的会话信息
        Json::Value sessionsArray(Json::arrayValue);
        for(auto& sessionId : sessionIds)
        {
            Json::Value sessionMsg;
            // 根据会话id找会话信息
            auto session = _sdk->getSessionInfo(sessionId);
            // 问：这里检测session是否为空是不是没有必要
            if(session)
            {
                sessionMsg["id"] = session->_id;
                sessionMsg["model"] = session->_mode_name;
                sessionMsg["created_at"] = static_cast<int64_t>(session->_createAt);
                sessionMsg["updated_at"] = static_cast<int64_t>(session->_updateAt);
                sessionMsg["message_count"] = session->_messages.size();
                // 问：为什么要获取第一条正文消息？ 正文消息为什么要用user来标识？
                if(!session->_messages.empty())
                    sessionMsg["first_user_message"] = session->_messages.front()._content;
                sessionsArray.append(sessionMsg);
            }      
        }

        // 构建完整回应
        Json::Value responseMsg(Json::objectValue);
        responseMsg["success"] = true;
        responseMsg["message"] = "get session lists success";
        responseMsg["data"] = sessionsArray;
        
        // 序列化
        Json::StreamWriterBuilder writer;
        std::string responseJson = Json::writeString(writer, responseMsg);

        response.status = 200;
        // set_content自动帮我们构建请求体的相关内容，包括content_length, content_type等
        // 而response.body = responseJson; 是将json字符串赋值给response.body，而不是构建请求体的相关内容
        response.set_content(responseJson,"application/json");
    }
    // 处理获取可用大模型列表的请求
    void ChatServer::handleGetModelLists(const httplib::Request& request, httplib::Response& response)
    {
        // 获取所有模型信息
        auto modelLists = _sdk->getModelInfo();
        // 构建json对象存放所有的模型信息
        Json::Value modelArray(Json::arrayValue);
        for(auto& model : modelLists)
        {
            Json::Value modelMsg;
            modelMsg["name"] = model._mode_name;
            modelMsg["desc"] = model._desc;
            modelArray.append(modelMsg);
        }

        // 构建完整回应
        Json::Value responseMsg(Json::objectValue);
        responseMsg["success"] = true;
        responseMsg["message"] = "get model lists success";
        responseMsg["data"] = modelArray;

        // 序列化
        Json::StreamWriterBuilder writer;
        std::string responseJson = Json::writeString(writer, responseMsg);

        // 构建请求体
        response.status = 200;
        response.set_content(responseJson,"application/json");
    }
    // 处理创建会话的请求
    void ChatServer::handleCreateSession(const httplib::Request& request, httplib::Response& response)
    {
        // 反序列化拿到需要用哪个模型创建会话
        Json::Value requestJson;
        Json::CharReaderBuilder readerBuilder;
        std::string errs;
        std::istringstream iss(request.body);

        // 使用 parseFromStream 替代 reader.parse
        if (!Json::parseFromStream(readerBuilder, iss, &requestJson, &errs))
        {
            // 客户端发送异常
            std::string errorMessage = buildResponse("parse request body failed: " + errs, false);
            response.status = 400;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 提取
        std::string modelName = requestJson["model"].asString();

        // 创建会话
        std::string sessionId = _sdk->createSession(modelName);
        if(sessionId.empty())
        {
            // 服务端创建失败
            std::string errorMessage = buildResponse("create session failed", false);
            response.status = 500;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 构建完整响应
        Json::Value dataJson;
        dataJson["session_id"] = sessionId;
        dataJson["model"] = modelName;
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "create session success";
        responseJson["data"] = dataJson;

        // 序列化
        Json::StreamWriterBuilder writer;
        std::string responseJsonStr = Json::writeString(writer, responseJson);

        // 构建请求体
        response.status = 200;
        response.set_content(responseJsonStr,"application/json");
    }
    // 处理删除会话的请求
    void ChatServer::handleDeleteSession(const httplib::Request& request, httplib::Response& response)
    {
        // /api/session/${session_id}
        // 由于请求中携带session_id，所以不需要反序列化请求，但需要将session_id从请求中提取出来
        std::string sessionId = request.matches[1].str();
        bool isSuccess = _sdk->deleteSession(sessionId);
        if(isSuccess)
        {
            // 服务端删除成功
            std::string successMessage = buildResponse("delete session success", true);
            response.status = 200;
            response.set_content(successMessage, "application/json");
        }
        else 
        {
            // 服务端删除失败
            std::string errorMessage = buildResponse("delete session failed", false);
            response.status = 500;
            response.set_content(errorMessage, "application/json");
        }
    }
    // 处理获取历史消息的请求
    void ChatServer::handleGetHistoryMessage(const httplib::Request& request, httplib::Response& response)
    {
        // 提取url中的session_id
        std::string session_id = request.matches[1].str();
        auto session = _sdk->getSessionInfo(session_id);
        if(session)
        {
            auto historyMessages = session->_messages;
            Json::Value historyArray(Json::arrayValue);
            for(auto& message : historyMessages)
            {
                Json::Value messageJson;
                messageJson["id"] = message._id;
                messageJson["role"] = message._role;
                messageJson["content"] = message._content;
                messageJson["timestamp"] = static_cast<int64_t>(message._timestamp);
                historyArray.append(messageJson);
            }
            // 构建完整响应
            Json::Value responseJson;
            responseJson["success"] = true;
            responseJson["message"] = "get history message success";
            responseJson["data"] = historyArray;
            // 序列化
            Json::StreamWriterBuilder writer;
            std::string responseJsonStr = Json::writeString(writer, responseJson);
            // 构建请求体
            response.status = 200;
            response.set_content(responseJsonStr,"application/json");
        }
        else
        {
            // 服务端获取失败
            std::string errorMessage = buildResponse("get history message failed", false);
            response.status = 500;
            response.set_content(errorMessage, "application/json");
        }
    }
    // 处理发送消息的请求 -- 全量发送
    void ChatServer::handleSendMessage(const httplib::Request& request, httplib::Response& response)
    {
        // 解析请求参数
        Json::Value requestJson;
        Json::CharReaderBuilder readerBuilder;
        std::string errs;
        std::istringstream ss(request.body);

        if(!Json::parseFromStream(readerBuilder, ss, &requestJson, &errs))
        {
            // 客户端发送异常
            std::string errorMessage = buildResponse("parse request body failed: " + errs, false);
            response.status = 400;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 1. 先检查键是否存在，且是不是字符串类型
        if (!requestJson.isMember("session_id") || !requestJson["session_id"].isString() ||
            !requestJson.isMember("message") || !requestJson["message"].isString()) {
            
            std::string errorMessage = buildResponse("missing or invalid session_id/message field", false);
            response.status = 400;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 2. 确认存在且是字符串后，再安全地提取
        std::string session_id = requestJson["session_id"].asString();
        std::string message = requestJson["message"].asString();
        if(session_id.empty() || message.empty())
        {
            // 客户端发送异常
            std::string errorMessage = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 发送消息
        std::string replayMessage = _sdk->sendMessage(session_id, message);
        if(replayMessage.empty())
        {
            // 服务端发送失败
            std::string errorMessage = buildResponse("send message failed", false);
            response.status = 500;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 构建完整应答
        Json::Value  dataJson;
        dataJson["session_id"] = session_id;
        dataJson["response"] = replayMessage;
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "send message success";
        responseJson["data"] = dataJson;
        // 序列化
        Json::StreamWriterBuilder writer;
        std::string responseJsonStr = Json::writeString(writer, responseJson);
        // 构建请求体
        response.status = 200;
        response.set_content(responseJsonStr,"application/json");
    }
    // 处理发送消息的请求 -- 增量发送
    void ChatServer::handleSendMessageStream(const httplib::Request& request, httplib::Response& response)
    {
        // 解析请求体
        Json::Value requestJson;
        Json::CharReaderBuilder readerBuilder;
        std::string errs;
        std::istringstream ss(request.body);

        if(!Json::parseFromStream(readerBuilder, ss, &requestJson, &errs))
        {
            // 客户端发送异常
            std::string errorMessage = buildResponse("parse request body failed: " + errs, false);
            response.status = 400;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 提取请求内容
        // 1. 先检查键是否存在，且是不是字符串类型
        if(!requestJson.isMember("session_id") || !requestJson.isMember("message") ||
            !requestJson["session_id"].isString() || !requestJson["message"].isString())
        {
            // 客户端发送异常
            std::string errorMessage = buildResponse("missing or invalid session_id/message field", false);
            response.status = 400;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 2. 确认存在且是字符串后，再安全地提取
        std::string session_id = requestJson["session_id"].asString();
        std::string message = requestJson["message"].asString();
        if(session_id.empty() || message.empty())
        {
            // 客户端发送异常
            std::string errorMessage = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errorMessage, "application/json");
            return;
        }

        // 准备流式响应
        response.status = 200;
        response.set_header("Cache-Control", "no-cache");              // 不使用缓存，服务器立即将数据发送到网络
        response.set_header("Connection", "keep-alive");               // 保持连接，服务器不会关闭连接
        response.set_header("Access-Control-Allow-Origin", "*");        // 允许跨域请求
        response.set_header("Access-Control-Allow-Headers", "*");      // 允许所有请求头
        // offset是服务端自动传递的参数，用于记录当前发送的位置，用于判断是否需要继续发送数据
        // 问：为什么只调用一次？
        response.set_chunked_content_provider("text/event-stream", [this,session_id,message](size_t offset, httplib::DataSink& dataSink){
            auto writechunk = [&](std::string chunk, bool end){
                // 将信息转为sse格式
                // 问：为什么要转化为sse？
                // 问：为什么要用valueToQuotedString？
                std::string ssedata = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";
               // 将构建好的sse信息发送给客户端
               dataSink.write(ssedata.c_str(), ssedata.size());
               // 处理结束标记
                if(end){
                    // 流向响应结束
                    std::string doneData = "data: [DONE]\n\n";
                    dataSink.write(doneData.c_str(), doneData.size());
                    dataSink.done();    // 表示流式响应结束
                    return false;       // 不再有后续数据
                }
                return true;
            };
            // 先给客户端发送一个空的数据块，避免客户端长时间的等待
            if (!writechunk("", false))
            {
                return false;
            }

            // 发送信息
            _sdk->sendMessageIncrement(session_id, message, writechunk);
            return false;
        });
    }

    // 设置路由规则
    void ChatServer::setRoutes()
    {
        // 处理创建会话请求
        _chatServer->Post("/api/session", [this](const httplib::Request& request, httplib::Response& response){
            handleCreateSession(request, response);
        });

        // 处理获取会话列表请求
        _chatServer->Get("/api/sessions", [this](const httplib::Request& request, httplib::Response& response){
            handleGetSessionLists(request, response);
        }); 

        // 处理获取模型列表请求
        _chatServer->Get("/api/models", [this](const httplib::Request& request, httplib::Response& response){
            handleGetModelLists(request, response);
        });

        // 处理删除会话请求
        _chatServer->Delete("/api/session/(.*)", [this](const httplib::Request& request, httplib::Response& response){
            handleDeleteSession(request, response);
        });

        // 处理获取历史消息请求
        _chatServer->Get("/api/session/(.*)/history", [this](const httplib::Request& request, httplib::Response& response){
            handleGetHistoryMessage(request, response);
        });

        // 处理发送消息请求-全量返回
        _chatServer->Post("/api/message", [this](const httplib::Request& request, httplib::Response& response){
            handleSendMessage(request, response);
        });

        // 处理发送消息请求-增量返回
        _chatServer->Post("/api/message/async", [this](const httplib::Request& request, httplib::Response& response){
            handleSendMessageStream(request, response);
        });
    }
} // end ai_chat_server