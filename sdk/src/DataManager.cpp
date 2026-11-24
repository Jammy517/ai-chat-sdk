#include "../include/DataManager.h"
#include "../include/util/log.h"
#include <cstdint>
#include <memory>
#include <mutex>

namespace ai_chat_sdk {
    using namespace Jammy;

    DataManager::DataManager(const std::string& dbName) 
    :_dbName(dbName),_db(nullptr)
    {
        // 创建会话数据库
        if(SQLITE_OK != sqlite3_open(_dbName.c_str(),&_db))
        {
            ERROR("打开数据库失败：{}", sqlite3_errmsg(_db));
            exit(1);
        }
        INFO("打开数据库成功：{}", dbName);

        // 开启外键约束（SQLite 默认 OFF，级联删除依赖它）
        executeSQL("PRAGMA foreign_keys = ON;");
        
        // 创建消息数据库
        if(!initSQLTablle())
        {
            sqlite3_close(_db);
            _db = nullptr;
            ERROR("初始化消息数据库失败");
        }
        INFO("初始化消息数据库表成功");
    }

    DataManager::~DataManager()
    {
        if(_db)
        {
            sqlite3_close(_db);
            _db = nullptr;
        }
    }
    // 初始化数据库表
    bool DataManager::initSQLTablle()
    {   
        std::lock_guard<std::mutex> lock(_mutex);
        // 构建会话sql语句
        const std::string insertSession = R"(
            create table if not exists sessions(
                session_id text primary key not null,
                model_name text not null,
                create_at integer not null,
                update_at integer not null
            );
        )";

        // 执行创建会话数据库
        if(!executeSQL(insertSession))
        {
            return false;
        }

        // 构建消息数据库
        const std::string insertMessage = R"(
            create table if not exists messages(
                message_id text primary key not null,
                session_id text not null,
                role text not null,
                content text not null,
                timestamp integer not null,
                foreign key(session_id) references sessions(session_id) on delete cascade
            );
        )";

        // 执行创建消息数据库
        if(!executeSQL(insertMessage))
        {
            return false;
        }
        return true;
    }
        
    bool DataManager::insertSession(const Session& session)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // 创建sql语句
        const std::string insertSession = R"(
            insert into sessions(session_id, model_name, create_at, update_at)
            values(?, ?, ?, ?);
        )";

        // 检查sql语句是否正确
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,insertSession.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("insertSession - 准备sql语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        // 绑定sql语句参数
        sqlite3_bind_text(stmt,1,session._id.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,2,session._mode_name.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt,3,session._createAt);
        sqlite3_bind_int64(stmt,4,session._updateAt);

        // 执行sql语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            ERROR("insertSession - 执行sql语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        // 释放sql语句
        sqlite3_finalize(stmt);
        INFO("insertSession - 插入会话成功：{}", session._id);
        return true;
    }
    // 从数据库中删除一个会话
    bool DataManager::deleteSession(const std::string& sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // 构建sql语句
        const std::string deleteSession = R"(
            delete from sessions where session_id = ?;
        )";

        // 检查sql语句
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,deleteSession.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("deleteSession - 准备sql语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        // 绑定sql
        sqlite3_bind_text(stmt,1,sessionId.c_str(),-1,SQLITE_TRANSIENT);

        // 执行sql语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            ERROR("deleteSession - 执行sql语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        // 释放sql语句
        sqlite3_finalize(stmt);
        INFO("deleteSession - 删除会话成功：{}", sessionId);
        return true;
    }
    // 获取一个会话信息
    std::shared_ptr<Session> DataManager::getSessionInfo(const std::string& sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // 创建sql语句
        const std::string getSessionInfo = R"(
            select session_id, model_name, create_at, update_at from sessions where session_id = ?;
        )";

        // 检查sql语句是否正确
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,getSessionInfo.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("getSessionInfo - 准备sql语句失败：{}", sqlite3_errmsg(_db));
            return nullptr;
        }

        // 绑定sql
        sqlite3_bind_text(stmt,1,sessionId.c_str(),-1,SQLITE_TRANSIENT);

        // 执行sql语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW)
        {
            ERROR("getSessionInfo - 执行sql语句失败：{}", sqlite3_errmsg(_db));
            return nullptr;
        }

        // 提取数据
        std::string modelName = (const char*)sqlite3_column_text(stmt,1);
        int64_t createAt = sqlite3_column_int64(stmt,2);
        int64_t updateAt = sqlite3_column_int64(stmt,3);

        // 创建会话对象
        std::shared_ptr<Session> session = std::make_shared<Session>(sessionId);

        session->_mode_name = modelName;
        session->_createAt = createAt;
        session->_updateAt = updateAt;

        // 释放sql语句
        sqlite3_finalize(stmt);
        return session;
    }
    // 更新会话的时间戳
    // 问：这里为什么要传入时间戳
    bool DataManager::updateSessionTimestamp(const std::string& sessionId, time_t timestamp)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 构建sql语句
        const std::string updateSessionStamp = R"(
            update sessions set update_at = ? where session_id = ?;
        )";

        // 检查sql语句是否正确
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,updateSessionStamp.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("updateSessionTimestamp - 准备sql语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        // 绑定sql语句参数
        sqlite3_bind_int64(stmt,1,static_cast<int64_t>(timestamp));
        sqlite3_bind_text(stmt,2,sessionId.c_str(),-1,SQLITE_TRANSIENT);

        // 执行sql语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            ERROR("updateSessionTimestamp - 执行sql语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        // 释放sql语句
        sqlite3_finalize(stmt);
        INFO("updateSessionTimestamp - 更新会话时间戳成功：{}", sessionId);
        return true;
    }
    // 获取所有会话信息,并按照更新时间降序排列
    std::vector<std::shared_ptr<Session>> DataManager::getAllSessionInfo()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // 存储所有会话信息
        std::vector<std::shared_ptr<Session>> sessions;
        // 构建sql语句
        const std::string getAllSessionInfo = R"(
            select session_id, model_name, create_at, update_at from sessions order by update_at desc;
        )";

        // 检查sql语句
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,getAllSessionInfo.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("getAllSessionInfo - 准备sql语句失败 : {}",sqlite3_errmsg(_db));
            return {};
        }

        // 执行sql语句
        rc = sqlite3_step(stmt);
        while(rc == SQLITE_ROW)
        {
            std::string session_id = (const char*)sqlite3_column_text(stmt,0);
            std::string model_name = (const char*)sqlite3_column_text(stmt,1);
            int64_t create_time = sqlite3_column_int64(stmt,2);
            int64_t update_time = sqlite3_column_int64(stmt,3);

            // 构建会话对象
            std::shared_ptr<Session> session = std::make_shared<Session>(session_id);
            session->_mode_name = model_name;
            session->_createAt = create_time;
            session->_updateAt = update_time;

            // 插入vector中
            sessions.push_back(session);
            rc = sqlite3_step(stmt);
        }

        // 释放sql语句
        sqlite3_finalize(stmt);
        return sessions;
    }
    // 获取所有的会话id
    std::vector<std::string> DataManager::getAllSessionId()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // 存储所有会话id
        std::vector<std::string> sessionIds;
        
        // 构建sql语句
        const std::string getAllSessionId = R"(
            select session_id from sessions;
        )";

        // 检查sql语句是否正确
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,getAllSessionId.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("getAllSessionId - 准备sql语句失败 : {}",sqlite3_errmsg(_db));
            return {};
        }

        // 执行sql语句
        rc = sqlite3_step(stmt);
        while(rc == SQLITE_ROW)
        {
            std::string session_id = (const char*)sqlite3_column_text(stmt,0);
            sessionIds.push_back(session_id);
            rc = sqlite3_step(stmt);
        }

        // 释放sql语句
        sqlite3_finalize(stmt);
        return sessionIds;
    }
    // 删除所有会话
    bool DataManager::deleteAllSession()
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 构建sql语句
        const std::string deleteAllSession = R"(
            delete from sessions;
        )";
        // sqlite3_step
        // // 检查sql语句是否正确
        // sqlite3_stmt* stmt;
        // int rc = sqlite3_prepare_v2(_db,deleteAllSession.c_str(),-1,&stmt,nullptr);
        // if(rc != SQLITE_OK)
        // {
        //     ERROR("deleteAllSession - 准备sql语句失败：{}", sqlite3_errmsg(_db));
        //     return false;
        // }

        // // 执行sql语句
        // rc = sqlite3_step(stmt);
        // if(rc != SQLITE_DONE)
        // {
        //     ERROR("deleteAllSession - 执行sql语句失败：{}", sqlite3_errmsg(_db));
        //     sqlite3_finalize(stmt);
        //     return false;
        // }

        // // 释放sql语句
        // sqlite3_finalize(stmt);
        // INFO("deleteAllSession - 删除所有会话成功");

        // sqlite3_exec
        if(sqlite3_exec(_db,deleteAllSession.c_str(),nullptr,nullptr,nullptr) != SQLITE_OK)
        {
            ERROR("deleteAllSession - 执行sql语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }
        INFO("deleteAllSession - 删除所有会话成功");
        return true;
    }
    // 获取会话总数
    size_t DataManager::getSessionNum()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // 存储会话总数
        size_t sessionNum = 0;

        // 构建sql语句
        const std::string getSessionNum = R"(
            select count(*) from sessions;
        )";

        // 检查sql语句是否正确
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,getSessionNum.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("getSessionNum - 准备sql语句失败 : {}",sqlite3_errmsg(_db));
            return 0;
        }

        // 执行sql语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW)
        {
            ERROR("getSessionNum - 执行sql语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return 0;
        }

        // 获取会话总数
        sessionNum = sqlite3_column_int64(stmt,0);

        // 释放sql语句
        sqlite3_finalize(stmt);
        return sessionNum;
    }

    // 消息数据
    // 获取指定会话的所有消息
    std::vector<Message> DataManager::getAllMessage(const std::string sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // 存储所有消息
        std::vector<Message> messages;

        // 构建sql语句
        const std::string getAllMessage = R"(
            select message_id,role,content,timestamp from messages where session_id = ?;
        )";

        // 检查sql语句
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,getAllMessage.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("getAllMessage - 准备sql语句失败 : {}",sqlite3_errmsg(_db));
            return {};
        }

        // 绑定语句
        rc = sqlite3_bind_text(stmt,1,sessionId.c_str(),-1,SQLITE_TRANSIENT);
        if(rc != SQLITE_OK)
        {
            ERROR("getAllMessage - 绑定sql语句失败 : {}",sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return {};
        }

        // 执行sql语句
        rc = sqlite3_step(stmt);
        while(rc == SQLITE_ROW)
        {
            // 获取参数
            std::string message_id = (const char*)sqlite3_column_text(stmt, 0);
            std::string role = (const char*)sqlite3_column_text(stmt, 1);
            std::string content = (const char*)sqlite3_column_text(stmt, 2);
            int64_t timestamp = sqlite3_column_int64(stmt, 3);

            // 构建Message
            Message msg(role, content);
            msg._id = message_id;
            msg._timestamp = timestamp;
            messages.push_back(msg);
            
            rc = sqlite3_step(stmt);
        }

        if(rc != SQLITE_DONE)
        {
            ERROR("getAllMessage - 执行sql语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return {};
        }

        // 释放sql语句
        sqlite3_finalize(stmt);
        return messages;
    }

    // 插入一条消息(指定哪一个会话)---更新会话时间戳
    bool DataManager::insertMessage(const std::string& sessionId, const Message& message)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 构建sql语句
        const std::string insertMessage = R"(
            insert into messages (message_id,session_id,role,content,timestamp)
            values (?,?,?,?,?);
        )";

        // 检查sql语句
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,insertMessage.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("insertMessage - 准备insertMessage sql语句失败 : {}",sqlite3_errmsg(_db));
            return false;
        }

        // 绑定参数到sql语句
        sqlite3_bind_text(stmt,1,message._id.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,2,sessionId.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,3,message._role.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,4,message._content.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt,5,message._timestamp);

        // 执行sql语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            ERROR("insertMessage - 执行insertMessage sql语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);

        // 同时更新会话时间戳
        // 构建sql语句
        const std::string updateSession = R"(
            update sessions set update_at = ? where session_id = ?;
        )";

        // 检查sql语句
        sqlite3_stmt* updatastmt;
        rc = sqlite3_prepare_v2(_db,updateSession.c_str(),-1,&updatastmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("insertMessage - 准备updateSession sql语句失败 : {}",sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }

        // 绑定参数到sql语句
        sqlite3_bind_int64(updatastmt,1,message._timestamp);
        sqlite3_bind_text(updatastmt,2,sessionId.c_str(),-1,SQLITE_TRANSIENT);

        // 执行sql语句
        rc = sqlite3_step(updatastmt);
        if(rc != SQLITE_DONE)
        {
            ERROR("insertMessage - 执行updateSession sql语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(updatastmt);
            return false;
        }

        sqlite3_finalize(updatastmt);
        return true;
    }

    // 删除所有消息(指定哪一个会话)
    bool DataManager::deleteAllMessage(const std::string& sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 构建sql语句
        const std::string deleteAllMessage = R"(
            delete from messages where session_id = ?;
        )";

        // 检查sql语句
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,deleteAllMessage.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("deleteAllMessage - 准备sql语句失败 : {}",sqlite3_errmsg(_db));
            return false;
        }

        // 绑定参数到sql语句
        sqlite3_bind_text(stmt,1,sessionId.c_str(),-1,SQLITE_TRANSIENT);

        // 执行sql语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            ERROR("deleteAllMessage - 执行deleteAllMessage sql语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        return true;
    }
    // 删除指定会话的指定消息
    bool DataManager::deleteMessage(const std::string& sessionId,const std::string& messageId)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 构建sql语句
        const std::string deleteMessage = R"(
            delete from messages where message_id = ? and session_id = ?;
        )";

        // 检查sql语句
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(_db,deleteMessage.c_str(),-1,&stmt,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("deleteMessage - 准备sql语句失败 : {}",sqlite3_errmsg(_db));
            return false;
        }

        // 绑定参数到sql语句
        sqlite3_bind_text(stmt,1,messageId.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,2,sessionId.c_str(),-1,SQLITE_TRANSIENT);

        // 执行sql语句
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_DONE)
        {
            ERROR("deleteMessage - 执行deleteMessage sql语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        return true;
    }
    // 执行SQL语句的工具函数
    bool DataManager::executeSQL(const std::string& sql)
    {
        // 判断数据库是否创建成功
        if(!_db)
        {
            ERROR("数据库未创建成功");
            return false;
        }

        // 执行sql语句
        int rc = sqlite3_exec(_db,sql.c_str(),nullptr,nullptr,nullptr);
        if(rc != SQLITE_OK)
        {
            ERROR("执行sql语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }
        return true;
    }
} // end ai_chat_sdk

