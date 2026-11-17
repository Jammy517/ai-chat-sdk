#include "../include/GeminiProvider.h"
#include "../include/util/log.h"
#include "jsoncpp/json/json.h"
#include "httplib.h"
#include <jsoncpp/json/reader.h>
#include <sstream>

namespace ai_chat_sdk
{
    using namespace Jammy;
    bool GeminiProvider::InitModel(const std::map<std::string,std::string>& Model_Config)
    {
        auto it = Model_Config.find("api_key");
        if(it == Model_Config.end())
        {
            ERROR("GeminiProvider: apikey is empty!");
            return false;
        }
        _apikey = it->second;

        it = Model_Config.find("base_url");
        if(it == Model_Config.end())
        {
            ERROR("GeminiProvider: base_url is empty!");
            return false;
        }
        _base_url = it->second;
        INFO("GeminiProvider: base_url set success!");
        INFO("GeminiProvider: apikey set success!");
        _isAvailable = true;
        return true;

    }
    std::string GeminiProvider::GetModelName() 
    {
        return "gemini-2.0-flash";
    }
    std::string GeminiProvider::GetModelDesc() 
    {
        return "Google 的急速响应模型，转为大模型部署和快速交互的场景设计";
    }
    bool GeminiProvider::IsAvailable()
    {
        return _isAvailable;
    }
    std::string GeminiProvider::SendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& request_param)
    {
        // 1.判断模型是否存活
        if(!_isAvailable)
        {
            ERROR("GeminiProvider: model is not available!");
            return "";
        }

        // 2.确定参数
        double temperature = 0.7;
        int max_tokens = 2048;
        auto it = request_param.find("temperature");
        if(it != request_param.end())
            temperature = std::stof(request_param.at("temperature"));
        it = request_param.find("max_tokens");
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
        Json::Value body;
        body["model"] = "gemini-3.5-flash";
        body["messages"] = messageArray;
        body["temperature"] = temperature;
        body["max_tokens"] = max_tokens;  // 由于Gemini没有直接显示可以省略

        // 5.序列化
        Json::StreamWriterBuilder writer;
        std::string json_string = Json::writeString(writer,body);

        // 6.构建http客户端
        httplib::Client client(_base_url);
        client.set_connection_timeout(30,0);
        client.set_read_timeout(60,0);
        client.set_proxy("192.168.159.1",7897);

        // 7.构建请求头
        httplib::Headers header = {
            {"Authorization", _apikey}
        };

        // 8.发送请求
        auto response = client.Post("/v1beta/openai/chat/completions",header,json_string,"application/json");
        if(!response)
        {
            ERROR("GeminiProvider: request failed!");
            return "";
        }

        // 9.反序列化
        Json::Value json_res;
        Json::CharReaderBuilder reader;
        std::string err;
        std::istringstream ss(response->body);
        if(Json::parseFromStream(reader,ss,&json_res,&err))
        {
            ERROR("GeminiProvider: parse json failed!");
            return "";
        }
        else
        {
            INFO("GeminiProvider: parse json success!");
        }

        // 10.解析响应体
         if(json_res.isMember("choices") &&
                json_res["choices"].isArray() &&
                !json_res["choices"].empty())
            {
                Json::Value json_choice = json_res["choices"];
                if(json_choice.isMember("message") &&
                    json_choice["message"].isObject() &&
                    json_choice["message"].isMember("content") &&
                    json_choice["message"]["content"].isString())
                {
                    std::string reply = json_choice["message"]["content"].asString();
                    return reply;
                }
            }

        // 解析失败返回错误信息
        ERROR("GeminiProvider: parse json failed!");
        return "";

    }
    std::string GeminiProvider::SendMessageStream(const std::vector<Message>& messages,
                                const std::map<std::string,std::string>& request_param,
                                std::function<void(const std::string&,bool)> callback)
    {
        // 1.判断模型是否存活
        if(!_isAvailable)
        {
            ERROR("GeminiProvider: model is not available!");
            return "";
        }

        // 2.确定参数
        double temperature = 0.7;
        int max_tokens = 2048;
        auto it = request_param.find("temperature");
        if(it != request_param.end())
            temperature = std::stof(request_param.at("temperature"));
        it = request_param.find("max_tokens");
            max_tokens = std::stoi(it->second);

        // 3.构建历史消息
        Json::Value messageArray(Json::arrayValue);
        for(auto message : messages)
        {
            Json::Value msg;
            msg["role"] = message._role;
            msg["content"] = message._content;
            messageArray.append(msg);
        }

        // 4.构建请求体
        Json::Value body;
        body["model"] = "gemini-3.5-flash";
        body["messages"] = messageArray;
        body["temperature"] = temperature;
        body["max_tokens"] = max_tokens;  
        body["stream"] = true;

        // 5.序列化
        Json::StreamWriterBuilder writer;
        std::string json_string = Json::writeString(writer,body);

        // 6.构建http客户端
        httplib::Client client(_base_url);
        client.set_connection_timeout(60,0);
        client.set_read_timeout(300,0);
        client.set_proxy("192.168.159.1",7897);

        // 7.构建请求头
        httplib::Headers header = {
            {"Authorization", _apikey},
            {"Content-Type", "application/json"},
            {"Accept", "text/event-stream"}
        };

        // 8.设置请求
        httplib::Request request;
        request.path = "/v1beta/openai/chat/completions";
        request.method = "POST";
        request.body = json_string;
        request.headers = header;

        // 构建流式变量
        std::string buffer;
        std::string fulldata;
        bool streamFinish = false;
        bool isError = false;
        std::string errorMessage;
        int statusCode = 0;

        request.response_handler = [&](const httplib::Response& res){
            statusCode = res.status;
            if(statusCode != 200)
            {
                isError = true;
                errorMessage = "HTTP Error:" + std::to_string(statusCode);
                return false;
            }
        };

        request.content_receiver = [&](const char* data, size_t data_length, size_t offset, size_t total_length){
            if(isError)
            {
                ERROR("Gemini API returned error: {}", errorMessage);
                return "";
            }

            buffer.append(data,data_length);
            INFO("buffer: {}", buffer);

            size_t pos = 0;
            while((pos = buffer.find("\n\n")) != std::string::npos)
            {
                std::string event = buffer.substr(pos);
                buffer.erase(0,pos+2);

                if(event.empty() || event == ":")
                    continue;
                
                if(buffer.compare(0,6,"data: ") == 0)
                {
                    std::string json_string = event.substr(6);
                    
                    if(json_string == "[DONE]")
                    {
                        streamFinish = true;
                        continue;
                    }
                }

                Json::Value chunk;
                Json::CharReaderBuilder reader;
                std::string errMessage;
                std::istringstream iss(json_string);
                if(!Json::parseFromStream((reader),iss,&chunk,&errMessage))
                {
                    ERROR("GeminiProvider: parse json failed! error: {}", errMessage);
                    continue;
                }

                if(chunk.isMember("choices") && chunk["choices"].isArray() && !chunk["choices"].empty())
                {
                        Json::Value choice = chunk["choices"][0];
                        if(choice.isMember("delta") && choice["delta"].isObject() && choice["delta"].isMember("content")){
                            std::string deltaContent = choice["delta"]["content"].asString();
                            fulldata += deltaContent;
                            callback(deltaContent, false);
                        }
                }
            }

            auto res = client.send(request);
            if(!res){
                ERROR("GeminiProvider::sendMessageStream: failed to send request, error: {}", to_string(res.error()));
                return "";
            }
            
            if(!streamFinish)
            {
                WARNING("stream ended without [DONE] marker");
                callback(fulldata, true);
            }

            
        };


    }

} // end ai_chat_sdk