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
            auto it = Model_Config.find("api_key");
            if(it == Model_Config.end())
            {
                ERROR("DeepSeekProvider: api_key is empty!");
                return false;
            }
            else
            {
                _apikey = it->second;
                INFO("DeepSeekProvider: api_key set success!");
            }

            // 查看base_url
            it = Model_Config.find("base_url");
            if(it == Model_Config.end())
            {
                ERROR("DeepSeekProvider: base_url is empty!");
                return false;
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
            return "⼀款实⽤性强、中⽂优化的通⽤对话助⼿，适合⽇常问答与创作";
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
            request_body["model"] = "deepseek-flash";
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
            return "";
        }
}