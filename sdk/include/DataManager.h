#pragma once
#include <sqlite3.h>
#include <string>
#include <mutex>
#include "common.h"
#include <memory>
#include <vector>

namespace ai_chat_sdk {
    class DataManager
    {
    public:
        DataManager(const std::string& dbName);
        ~DataManager();

        // 会话数据
        // 向数据库中插入一个会话
        bool insertSession(const Session& session);
        // 从数据库中删除一个会话
        bool deleteSession(const std::string& sessionId);
        // 获取一个会话信息
        std::shared_ptr<Session> getSessionInfo(const std::string& sessionId);
        // 更新会话的时间戳
        // 问：这里为什么要传入时间戳
        bool updateSessionTimestamp(const std::string& sessionId, time_t timestamp);
        // 获取所有会话信息
        std::vector<std::shared_ptr<Session>> getAllSessionInfo();
        // 获取所有的会话id
        std::vector<std::string> getAllSessionId();
        // 删除所有会话
        bool deleteAllSession();
        // 获取会话总数
        size_t getSessionNum();

        // 消息数据
        // 获取指定会话的所有消息
        std::vector<Message> getAllMessage(const std::string sessionId);
        // 插入一条消息(指定哪一个会话)---更新会话时间戳
        bool insertMessage(const std::string& sessionId, const Message& message);
        // 删除所有消息(指定哪一个会话)
        bool deleteAllMessage(const std::string& sessionId);
        // 删除指定会话的指定消息
        bool deleteMessage(const std::string& sessionId,const std::string& messageId);
    private:
        // 初始化数据库表
        bool initSQLTablle();
        // 执行SQL语句的工具函数
        bool executeSQL(const std::string& sql);
    private:
        sqlite3* _db;
        std::string _dbName;
        std::mutex _mutex;
    };
} // end ai_chat_sdk

