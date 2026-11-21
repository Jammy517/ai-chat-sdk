#include "../include/DeepSeekProvider.h"
#include "../include/util/log.h"
#include "jsoncpp/json/json.h"
#include "httplib.h"
#include <sstream>

namespace ai_chat_sdk
{
    using namespace Jammy;
    // map存放的是大模型的配置信息的变量和配置信息对应的具体内容
    bool DeepSeekProvider::InitModel(const std::map<std::string,std::string>& Model_Config)
    {
        // 查看api_key
        auto it = Model_Config.find("apikey");
        if(it == Model_Config.end())
        {
            ERROR("DeepSeekProvider: apikey is empty!");
            return false;
        }
        else
        {
            _apikey = it->second;
            INFO("DeepSeekProvider: apikey set success!");
        }

        // 查看base_url
        it = Model_Config.find("base_url");
        if(it == Model_Config.end())
        {
            _base_url = "https://api.deepseek.com"
        }
        else
        {
            _base_url = it->second;
            INFO("DeepSeekProvider: base_url set success!");
        }

        INFO("init success!!!");
        _isAvailable = true;
        return true;
    }
    std::string DeepSeekProvider::GetModelName()
    {
        return "deepseek-flash";
    }
    std::string DeepSeekProvider::GetModelDesc()
    {
        return "⼀款实用性强、中文优化的通用对话助手，适合日常问答与创作";
    }
    bool DeepSeekProvider::IsAvailable()
    {
        return _isAvailable;
    }
    std::string DeepSeekProvider::SendMessage(const std::vector<Message>& messages, const std::map<std::string,std::string>& request_param)
    {
        // 1.判断模型是否初始化可用
        if(!IsAvailable())
        {
            ERROR("DeepSeekProvider: model is not init!");
            return "";
        }

        // 2.设置关键模型参数
        // 问：为什么要设置这两个参数，直接使用map中的不可以吗
        double temperature = 0.7;
        int max_tokens = 2048;
        auto it = request_param.find("temperature");
        if(it != request_param.end())
        {
            // 问：为什么要用at函数，使用it->second不可以吗
            temperature = stof(request_param.at("temperature"));
        }
        it = request_param.find("max_tokens");
        if(it != request_param.end())
        {
            max_tokens = stoi(it->second);
        }

        // 3.构建历史消息
        // 问：arrayvalue是什么?
        Json::Value message_array(Json::arrayValue);  // 创建一个字串数组
        for(auto& message : messages)
        {
            Json::Value msg;  // 构建单个字串
            msg["role"] = message._role;
            msg["content"] = message._content;
            message_array.append(msg);  // 将单个字串放入数组
        }

        // 4.构建请求体
        Json::Value request_body;
        request_body["model"] = GetModelName();
        request_body["messages"] = message_array;
        request_body["max_tokens"] = max_tokens;
        request_body["temperature"] = temperature;

        // 5.序列化
        Json::StreamWriterBuilder writer;
        std::string json_string = Json::writeString(writer,request_body);

        // 6.构建http client请求
        httplib::Client client(_base_url);
        // 问：两个参数是什么意思，可用不用设置吗
        client.set_connection_timeout(30,0);  // 30秒连接超时
        client.set_read_timeout(60,0);  //60秒读取时长

        // 7.设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apikey}
        };

        // 8.发送请求
        // 问：post的返回值
        auto response = client.Post("/chat/completions",headers,json_string,"application/json");
        // 问：这里的判断是干什么的
        if(!response)
        {
            ERROR("Failed to connect to DeepSeep API – check network and SSL") ;
            return "" ;
        }
        DEBUG("DeepSeek API response status: {}", response->status);
        DEBUG("DeepSeek API response body: {}", response->body);
        // 检查响应是否成功
        if(response->status != 200)
        {
            ERROR("DeepSeek API returned non-200 status: {} - {}", response->status,response->body);
            return "";
        }

        // 9.反序列化
        Json::Value response_json;
        Json::CharReaderBuilder reader;
        std::string error;
        // 问：istringstream的作用
        std::istringstream response_stream(response->body);
        if(!Json::parseFromStream(reader,response_stream,&response_json,&error))
        {
            ERROR("Failed to parse DeepSeek API response: {}", error);
            return "";  
        }

        // 10.解析大模型返回的信息
        // 问：为什么必须判断choice是否是数组
        if(response_json.isMember("choices") && response_json["choices"].isArray() && !response_json["choices"].empty())
        {
            auto& choice = response_json["choices"][0];
            if(choice.isMember("message") && choice["message"].isMember("content"))
            {
                std::string reply_content = choice["message"]["content"].asString();
                INFO("Received DeepSeek response: {}", reply_content);
                return reply_content;
            }
        }
        // 解析失败，返回错误信息
        ERROR("Invalid response format from DeepSeek API");
        return "Invalid response format from DeepSeek API";

    }
    std::string DeepSeekProvider::SendMessageStream(const std::vector<Message>& messages, 
                                const std::map<std::string,std::string>& request_param,
                                std::function<void(const std::string&,bool)> callback)
    {
        // 1.判断模型是否存活
        if(!IsAvailable())
        {
            ERROR("DeepSeekProvider: model is not init!");
            return "";
        }

        // 2.设置参数
        double temperature = 0.7;
        int max_tokens = 2048;
        auto it = request_param.find("temperature");
        if(it != request_param.end())
        {
            temperature = stof(it->second);
        }
        it = request_param.find("max_tokens");
        if(it != request_param.end())
        {
            max_tokens = stoi(it->second);
        }

        // 3.构建历史对象
        Json::Value message_array(Json::arrayValue);
        for(auto& message : messages)
        {
            Json::Value msg;
            msg["role"] = message._role;
            msg["content"] = message._content;
            message_array.append(msg);
        }

        // 4.构建请求体
        Json::Value message_body;
        message_body["model"] = GetModelName();
        message_body["messages"] = message_array;
        message_body["temperature"] = temperature;
        message_body["max_tokens"] = max_tokens;
        message_body["stream"] = true;

        // 5.序列化
        Json::StreamWriterBuilder writer;
        std::string json_string = Json::writeString(writer,message_body);

        // 6.构建http客户端
        httplib::Client client(_base_url);
        client.set_connection_timeout(30,0);
        client.set_read_timeout(300,0);

        // 7.构建请求头
        httplib::Headers header = {
            {"Authorization", "Bearer " + _apikey},
            {"Content-Type", "application/json"},   // send中不会自动带，需要手动添加
            {"Accept", "text/event-stream"}  //告诉服务器“我要 SSE，不要普通 JSON”
        };

        // 8.构建请求
        httplib::Request req;
        req.method = "POST";
        req.path = "/chat/completions";
        req.headers = header;
        req.body = json_string;

        // 构建流式相应变量
        std::string buffer;         // 单个流式内容
        // 问：这里的fullResponse可以不需要吗，在外面接收
        std::string fullResponse;   // 完整内容
        // 问：下面的变量可以不要吗
        bool isError = false;       // 响应是否成功
        std::string errorMessage;          // 错误信息
        int statusCode = 0;         // 状态码
        bool streamFinish = false;  // 用来检测流式响应是否结束
        
        // 问：这里的&是捕捉所有的变量吗
        // 问：这里是处理的是响应内容吗
        // 问：这里的的函数作用是什么
        req.response_handler = [&](const httplib::Response& response){
            statusCode = response.status;
            if(statusCode != 200)
            {
                isError = true;
                errorMessage = "HTTP Error:" + std::to_string(statusCode);
                return false;
            }
            return true;
        };

        // 问：这里面的参数是什么意思
        // 问：这里的参数是服务器填入的吗
        // 问：每次服务器返回都会自动调用该函数吗
        req.content_receiver = [&](const char* data,size_t data_length, size_t offset, size_t total_length){
            // 问：为什么要判断是否错误
            if(isError)
            {
                ERROR("DeepSeek API returned error: {}", errorMessage);
                return false;
            }

            // 将数据添加到buffer中然后再去除单个响应
            // 问：为什么不循环添加
            buffer.append(data,data_length);
            INFO("buffer: {}", buffer);

            // 循环处理每一个事件
            // 问：为什么要找\n\n
            size_t pos = 0;
            while((pos = buffer.find("\n\n")) != std::string::npos)
            {
                // 提取单个响应体   
                std::string event = buffer.substr(0,pos);
                buffer.erase(0,pos + 2);

                // 问：这里为什么要这样做
                if(event.empty() || event[0] == ':')
                    continue;

                // 提取数据字段
                if(event.compare(0,6,"data: ") == 0)   
                {
                    std::string json_string = event.substr(6);

                    // 判断是否是最后一个执行流
                    if(json_string == "[DONE]")
                    {
                        streamFinish = true;
                        callback("",true);
                        return true;
                    }

                    // 进行反序列化
                    Json::Value chunk;
                    Json::CharReaderBuilder reader;
                    std::string error;
                    std::istringstream jsonStream(json_string);

                    bool ok = Json::parseFromStream(reader,jsonStream,&chunk,&error);
                    if(ok)
                    {
                        if(chunk.isMember("choices") &&
                            chunk["choices"].isArray() &&
                            !chunk["choices"][0].empty() &&
                            chunk["choices"][0]["delta"].isMember("content"))
                        {
                            std::string content = chunk["choices"][0]["delta"]["content"].asString();
                            fullResponse += content;
                            callback(content,false);
                        }
                    }
                    else
                    {
                        ERROR("DeepSeek API returned error: {}", error);
                    }
                }

            }
            return true;
        };

        // 9.发送请求
        httplib::Result result = client.send(req);
        // 问：这里为什么还要判断,response_handler不是已经判断过了吗
        if(!result)
        {
            // 请求连接失败，⽹络问题、DNS解析失败等
            auto err = result.error();
            // 问：为什么这里要将err进行转化
            ERROR("Network error : {}", httplib::to_string(err));
            return "";              
        }

        // 确保流正常结束
        if(!streamFinish)
        {
            WARNING("Stream ended without [DONE] marker");
            // 问：callback的作用
            callback("", true);
        }

        return fullResponse;
    }
}   