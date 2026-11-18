#include "../include/OllamaProvider.h"
#include "../include/util/log.h"
#include "jsoncpp/json/json.h"
#include "httplib.h"
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <resolv.h>
#include <sstream>

namespace ai_chat_sdk
{
    using namespace Jammy;
    bool OllamaProvider::InitModel(const std::map<std::string, std::string> &Model_Config)
    {
        auto it = Model_Config.find("base_url");
        if(it == Model_Config.end())
        {
            ERROR("base_url is not found in Model_Config");
            return false;
        }
        _base_url = it->second;

        it = Model_Config.find("model_name");
        if(it == Model_Config.end())
        {
            ERROR("model_name is not found in Model_Config");
            return false;
        }
        _modelName = it->second;

        it = Model_Config.find("model_desc");
        if(it == Model_Config.end())
        {
            ERROR("model_desc is not found in Model_Config");
            return false;
        }
        _modelDesc = it->second;
        _isAvailable = true;
        return true;
    }

    std::string OllamaProvider::GetModelName() 
    {
        return _modelName;
    }

    std::string OllamaProvider::GetModelDesc() 
    {
        return _modelDesc;
    }

    bool OllamaProvider::IsAvailable()
    {
        return _isAvailable;
    }

    std::string OllamaProvider::SendMessage(const std::vector<Message> &messages, const std::map<std::string, std::string> &request_param)
    {
        // 1.判断模型是否存活
        if(!IsAvailable())
        {
            ERROR("OllamaProvider: model is not available!");
            return "";
        }

        // 2.确认参数
        double temperature = 0.7;
        int max_tokens = 2048;
        auto it = request_param.find("temperature");
        if(it != request_param.end())
            temperature = std::stof(it->second);
        it = request_param.find("max_tokens");
        if(it != request_param.end())
            max_tokens = std::stoi(it->second);

        // 3.构建历史会话
        Json::Value messageArray(Json::arrayValue);
        for(auto message : messages)
        {
            Json::Value msg;
            msg["role"] = message._role;
            msg["content"] = message._content;
            messageArray.append(msg);
        }

        // 4.构建请求体
        Json::Value options;
        options["temperature"] = temperature;
        options["num_ctx"] = max_tokens;
        Json::Value body;
        body["model"] = GetModelName();
        body["messages"] = messageArray;
        body["options"] = options;
        body["stream"] = false;

        // 5.序列化
        Json::StreamWriterBuilder writer;
        auto json_string = Json::writeString(writer,body);

        // 6.构建客户端
        httplib::Client client(_base_url.c_str());
        client.set_connection_timeout(30,0);
        client.set_read_timeout(60,0);

        // 7.构建请求头
        httplib::Headers header = {};
        // 8.发送请求
        auto response = client.Post("/api/chat",header,json_string,"application/json");
        if(!response)
        {
            ERROR("OllamaProvider: post request failed!");
            return "";
        }

        INFO("OllamaLLMProvider::sendMessage: response status: {}", response->status);
        INFO("OllamaProvider: post request success! response body: {}", response->body);

        if(response->status != 200){
            ERROR("OllamaLLMProvider::sendMessage: failed to send request, status: {}", response->status);
            return "";
        }
        // 9.反序列化
        Json::Value response_body;
        Json::CharReaderBuilder reader;
        std::string errMessage;
        std::istringstream ss(response->body);
        if(!Json::parseFromStream(reader,ss,&response_body,&errMessage))
        {
            ERROR("OllamaLLMProvider::sendMessage: failed to parse response body, error: {}", errMessage);
            return "";
        }

        // 10.提取
        if(response_body.isMember("message") && response_body["message"].isObject() && response_body["message"].isMember("content")){
            std::string modelResponse = response_body["message"]["content"].asString();
            INFO("OllamaLLMProvider::sendMessage: modelResponse: {}", modelResponse);
            return modelResponse;
        }

         // 处理其他情况
        ERROR("OllamaLLMProvider::sendMessage: invalid response format");
        return "";
    }

    std::string OllamaProvider::SendMessageStream(const std::vector<Message> &messages, 
                                                    const std::map<std::string, std::string> &request_param, 
                                                    std::function<void (const std::string &, bool)> callback)
    {
        // 1.判断模型是否存活
        if(!IsAvailable())
        {
            ERROR("OllamaProvider: model is not available!");
            return "";
        }

        // 2.确认参数
        double temperature = 0.7;
        int max_tokens = 2048;
        auto it = request_param.find("temperature");
        if(it != request_param.end())
            temperature = std::stof(it->second);
        it = request_param.find("max_tokens");
        if(it != request_param.end())
            max_tokens = std::stoi(it->second);

        // 3.构建历史会话
        Json::Value messageArray(Json::arrayValue);
        for(auto message : messages)
        {
            Json::Value msg;
            msg["role"] = message._role;
            msg["content"] = message._content;
            messageArray.append(msg);
        }
        
        // 4.构建请求体
        Json::Value options;
        options["temperature"] = temperature;
        options["num_ctx"] = max_tokens;
        Json::Value body;
        body["model"] = GetModelName();
        body["messages"] = messageArray;
        body["options"] = options;
        body["stream"] = true;

        // 5.序列化
        Json::StreamWriterBuilder writer;
        auto json_string = Json::writeString(writer,body);

        // 6.构建客户端
        httplib::Client client(_base_url.c_str());
        client.set_connection_timeout(30,0);
        client.set_read_timeout(300,0);

        // 7.构建请求头
        httplib::Headers header = {
            {"Accept", "text/event-stream"},
            {"Content-Type", "application/json"}
        };

        // 8.构建请求
        httplib::Request request;
        request.path = "/api/chat";
        request.body = json_string;
        request.headers = header;
        request.method = "POST";

        // 构建流式变量
        std::string buffer;
        bool isError = false;
        std::string errorMessage;
        int statusCode = 0;
        std::string fulldata;
        bool streamFinish = false;

       request.response_handler = [&](const httplib::Response& res)
       {
            statusCode = res.status;
            if(statusCode != 200)
            {
                isError = true;
                errorMessage = res.body;
                return false; 
            }
            return true;
       };

       request.content_receiver = [&](const char* data,size_t data_length,size_t offset,size_t total_length)
       {
            if(isError)
            {
                return false;
            }

            buffer.append(data,data_length);
            size_t pos;
            while((pos = buffer.find("\n")) != std::string::npos)
            {
                std::string chunk = buffer.substr(0,pos);
                buffer.erase(0,pos+1);
               
                if(chunk.empty())
                {
                    continue;
                }   
                
                // 反序列化
                Json::Value json_chunk;
                Json::CharReaderBuilder reader;
                std::string ErrorMessage;
                std::istringstream ss(chunk);
                if(!Json::parseFromStream(reader,ss,&json_chunk,&ErrorMessage))
                {
                    ERROR("OllamaProvider::SendMessageStream: failed to parse chunk, error: {}", ErrorMessage);
                    continue;
                }

                // 判断是否流式响应结束
                if(json_chunk.isMember("done") && json_chunk["done"].asBool())
                {
                    streamFinish = true;
                    callback("",true);
                    return true;
                }

                if(json_chunk.isMember("message") && json_chunk["message"].isObject() && json_chunk["message"].isMember("content"))
                {
                    std::string modelResponse = json_chunk["message"]["content"].asString();
                    INFO("OllamaLLMProvider::sendMessage: modelResponse: {}", modelResponse);
                    fulldata += modelResponse;
                    callback(modelResponse,false);
                }
            }
          
            return true;
       };

       // 给Ollama服务器发送请求
        auto response = client.send(request);
        if(!response){
            ERROR("OllamaLLMProvider::sendMessageStream: failed to send request, error: {}", to_string(response.error()));
            return "";
        }

        // 确保流式响应正常结束
        if(!streamFinish){
            ERROR("OllamaLLMProvider::sendMessageStream: stream not finish, fullData: {}", fulldata);
            callback("", true);
        }

        return fulldata;

        return "";
    }

} // end ai_chat_sdk