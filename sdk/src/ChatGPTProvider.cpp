#include "../include/util/log.h"
#include "../include/ChatGPTProvider.h"
#include <httplib.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

namespace ai_chat_sdk
{
    using namespace Jammy;
        bool ChatGPTProvider::InitModel(const std::map<std::string,std::string>& Model_Config) 
        {
            auto it = Model_Config.find("apikey");
            if(it != Model_Config.end())
            {
                _apikey = it->second;
                INFO("ChatGPTProvider: apikey set success!");
            }
            else
            {
                ERROR("ChatGPTProvider: apikey is empty!");
                return false;
            }

            it = Model_Config.find("base_url");
            if(it != Model_Config.end())
            {
                _base_url = it->second;
                INFO("ChatGPTProvider: base_url set success!");
            }
            else
            {
                ERROR("ChatGPTProvider: base_url is empty!");
                return false;
            }

            _isAvailable = true;
            return true;
        }
        std::string ChatGPTProvider::GetModelName()
        {
            return "gpt-4o-mini";
        }
        std::string ChatGPTProvider::GetModelDesc() 
        {
            return "OpenAI 推出的轻量级、高性价比模型，核心能力解决 GPT-4 Turbo，但更经济";
        }
        bool ChatGPTProvider::IsAvailable()
        {
            return _isAvailable;
        }
        std::string ChatGPTProvider::SendMessage(const std::vector<Message>& messages, const std::map<std::string,std::string>& request_param) 
        {
            // 1.判断模型是否能存活
            if(!IsAvailable())
            {
                ERROR("ChatGPTProvider: model is not available!");
                return "";
            }

            // 2.设置参数
            double temperature = 0.7;
            int max_output_tokens = 2048;
            auto it = request_param.find("temperature");
            if(it != request_param.end())
            {
                temperature = std::stod(it->second);
            }
            it = request_param.find("max_output_tokens");
            if(it != request_param.end())
            {
                max_output_tokens = std::stoi(it->second);
            }

            // 3.构建历史会话
            Json::Value messageArray(Json::arrayValue);
            for(auto& msg : messages)
            {
                Json::Value message(Json::objectValue);
                message["role"] = msg._role;
                message["content"] = msg._content;
                messageArray.append(message);
            }

            // 4.构建请求体
            Json::Value requestBody;
            requestBody["model"] = GetModelName();
            requestBody["input"] = messageArray;
            requestBody["temperature"] = temperature;
            requestBody["max_output_tokens"] = max_output_tokens;

            // 5.序列化
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "  ";
            std::string json_string = Json::writeString(writer, requestBody);

            // 6.创建http 客户端
            httplib::Client client(_base_url);
            client.set_connection_timeout(30,0);
            client.set_read_timeout(60,0);
            client.set_proxy("127.0.0.1",7890);        // todo: 配置代理服务器

            // 7.构建请求头
            httplib::Headers headers = 
            {
                {"Authorization", "Bearer " + _apikey} 
            };

            // 8.发送请求
            auto response = client.Post("/v1/responses",headers,json_string,"application/json");
            if(!response)
            {
                ERROR("Failed to connect to ChatGPT API, error: {}", httplib::to_string(response.error()));
                return "" ;
            }
            DEBUG("ChatGPT API response status: {}", response->status);
            DEBUG("ChatGPT API response body: {}", response->body);
            // 检查响应是否成功
            if(response->status != 200)
            {
                ERROR("ChatGPT API returned non-200 status: {} - {}", response->status,response->body);
                return "";
            }
            
            // 9.反序列化
            Json::Value response_json;
            Json::CharReaderBuilder reader;
            std::istringstream ss(response->body);
            std::string error;
            if(!Json::parseFromStream(reader,ss,&response_json,&error))
            {
                ERROR("Failed to parse ChatGPT API response: {}", error);
                return ""; 
            }

            // 10.提取content
            if(response_json.isMember("output") &&
                response_json["output"].isArray() &&
                !response_json["output"].empty() && 
                response_json["output"][0].isMember("content") &&
                response_json["output"][0]["content"].isArray() &&
                !response_json["output"][0]["content"].empty())
            {
                Json::Value json_content = response_json["output"][0]["content"][0];
                if(json_content.isMember("text") && json_content["text"].isString())
                {
                    std::string reply = json_content["text"].asString();
                    return reply;
                }
            }

            // 解析失败，返回错误信息
            ERROR("Invalid response format from ChatGPT API");
            return "";
        }
        std::string ChatGPTProvider::SendMessageStream(const std::vector<Message>& messages, 
                                  const std::map<std::string,std::string>& request_param,
                                  std::function<void(const std::string&,bool)> callback)
        {
            // 1.判断模型是否能存活
            if(!IsAvailable())
            {
                ERROR("ChatGPTProvider: model is not available!");
                return "";
            }

            // 2.设置参数
            double temperature = 0.7;
            int max_output_tokens = 2048;
            auto it = request_param.find("temperature");
            if(it != request_param.end())
            {
                temperature = std::stod(it->second);
            }
            it = request_param.find("max_output_tokens");
            if(it != request_param.end())
            {
                max_output_tokens = std::stoi(it->second);
            }

            // 3.构建历史会话
            Json::Value messageArray(Json::arrayValue);
            for(auto& msg : messages)
            {
                Json::Value message(Json::objectValue);
                message["role"] = msg._role;
                message["content"] = msg._content;
                messageArray.append(message);
            }

            // 4.构建请求体
            Json::Value requestBody;
            requestBody["model"] = GetModelName();
            requestBody["input"] = messageArray;
            requestBody["temperature"] = temperature;
            requestBody["max_output_tokens"] = max_output_tokens;
            requestBody["stream"] = true;

            // 5.序列化
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "  ";
            std::string json_string = Json::writeString(writer, requestBody);

            // 6.创建http 客户端
            httplib::Client client(_base_url);
            client.set_connection_timeout(30,0);
            client.set_read_timeout(60,0);
            client.set_proxy("127.0.0.1",7890);        // todo: 配置代理服务器

            // 7.构建请求头
            httplib::Headers headers = {
                {"Authorization", "Bearer " + _apikey},
                {"Content-Type", "application/json"},
                {"Accept", "text/event-stream"}
            };

            // 8.构建请求
            httplib::Request request;
            request.path = "/v1/responses";
            request.method = "POST";
            request.body = json_string;
            request.headers = headers;

            // 定义流式提取变量
            std::string buffer;
            std::string fullMessage;
            bool getError = false;
            bool streamFinish = false;
            std::string ErrorMsg;
            int statusCode = 0;

            request.response_handler = [&](const httplib::Response& res){
                statusCode = res.status;  
                if(statusCode != 200)
                {
                    getError = true;
                    ErrorMsg = res.body;
                    ERROR("ChatGPTProvider: API returned non-200 status: {} - {}", res.status,ErrorMsg);
                    return false;
                }
                return true;
            };

            request.content_receiver = [&](const char* data,size_t data_length,size_t offset,size_t total_length){

                // 如果HTTP请求失败，不用继续接收后续数据
                if(getError)
                {
                    ERROR("ChatGPTProvider API returned error: {}", ErrorMsg);
                    return false;
                }

                // 将本次的响应内容放入buffer中进行切割
                buffer.append(data,data_length);

                size_t pos = 0;
                while((pos = buffer.find("\n\n")) != std::string::npos)
                {
                    // 先解析出一个完整的json请求
                    std::string event = buffer.substr(0,pos);
                    buffer.erase(0,pos + 2);

                    // 定义变量存放ChatGPT返回的不同内容
                    // 问：为什么要放在istringstream中
                    std::istringstream ss(event);
                    std::string eventType;
                    std::string jsonStr;
                    std::string line;
                    while(std::getline(ss,line))
                    {
                        // 每次读取一行然后判断什么类型放在不同的位置
                        if(line.compare(0,6,"event:") == 0)
                            eventType = line.substr(7); // 去掉event:后面的空格，空格后面才是真正的json串
                        else if(line.compare(0,5,"data:") == 0)
                            jsonStr = line.substr(6);
                    }

                    // 处理事件
                    Json::Value chunk;
                    Json::CharReaderBuilder reader;
                    std::istringstream jsonStream(jsonStr);
                    std::string errs;
                    if(!Json::parseFromStream(reader,jsonStream,&chunk,&errs))
                    {
                        WARNING("Stream ChatGPT JSON parse error {}", errs);
                        continue;
                    } 

                    if(eventType == "response.output_text.delta")
                    {
                        // 每次解析的结果返回给上层，让用户看起来是个流式响应
                        if(chunk.isMember("delta") && chunk["delta"].isString())
                        {
                            std::string data = chunk["delta"].asString();
                            callback(data,false);
                        }
                    }
                    else if (eventType == "response.output_item.done")
                    {
                        // 将完整字串放入fullMessage返回
                        if(chunk.isMember("item") && chunk["item"].isObject())
                        {
                            Json::Value json_item = chunk["item"];
                            if(json_item.isMember("content") && json_item["content"].isArray() && !json_item["content"].empty())
                            {
                                if(json_item["content"][0].isMember("text") &&
                                    json_item["content"][0]["text"].isString())
                                {
                                    fullMessage += json_item["content"][0]["text"].asString();
                                }
                            }
                        }
                    }
                    else if(eventType == "response.completed")
                    {
                        // 表明流结束，设置流信息
                        callback("", true);
                        streamFinish = true;
                        return true;
                    }
                }
                return true;
            };

            // 9.发送请求
            auto result = client.send(request);
            if(!result)
            {
                auto err = result.error();
                ERROR("Network error : {}", httplib::to_string(err));
                return "";              
            }

            // 10.判断响应是否结束
            if(!streamFinish)
            {
                WARNING("Stream ended without [DONE] marker");
                callback("", true);
            }
            
            return fullMessage;
        }

} // end namespace 