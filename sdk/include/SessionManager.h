#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "common.h"
#include <mutex>
#include <atomic>
#include "DataManager.h"
#include <vector>

namespace ai_chat_sdk
{
    class SessionManager
    {
    public:
        // 构造函数，在开启会话管理的时候就直接开启数据管理，这样对会话和信息的操作直接可以插入数据库
        SessionManager(const std::string& dbName);
        // 创建会话：通过模型创建会话，并返回会话id
        std::string initSession(std::string& model_name);
        // 通过会话id返回会话信息
        std::shared_ptr<Session> getSession(std::string& sessionId);
        // 将消息添加到会话中
        bool addMessage(const std::string& sessionId, const Message& message);
        // 获取该会话中的所有历史消息
        std::vector<Message> getHistoryMessage(std::string sessionId);
        // 获取会话列表，返回所有会话
        // 问：为什么返回会话id不反回会话对象
        std::vector<std::string> getSessionLists(); 
        // 删除会话
        void deleteSession(std::string sessionId);
        // 删除所有会话
        void deleteAllSession();
        // 更新会话时间戳
        void updateSessionTimestamp(const std::string& session_id);
        // 获取会话个数
        size_t sessionTotal();
    private:
        std::string generateSessionId();  // 生成会话id
        // 问：这里为什么参数是这个，这个参数谁来传，不是记录的会话id为什么还能传参数
        std::string generateMessageId(size_t MessageCounter);  // 生成每个会话中的消息id
    private:
        // key:会话id value：会话信息
        std::unordered_map<std::string,std::shared_ptr<Session>> _sessions;
        // 问：为什么要加锁
        std::mutex _mutex;
        // 设置会话个数
        // 问：为什么要用atomic?atomic的用法
        std::atomic<int64_t> _sessionCounter = {0};
        // 创建数据管理类，将数据写入数据库
        DataManager _dataManager;
    };
} // end ai_chat_sdk