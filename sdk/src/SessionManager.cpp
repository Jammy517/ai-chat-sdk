#include "../include/SessionManager.h"
#include <sstream>
#include <iomanip>
#include "../include/util/log.h"
#include <algorithm>

namespace ai_chat_sdk
{
    using namespace Jammy;
    // 构造函数
    SessionManager::SessionManager(const std::string& dbName)
    : _dataManager(dbName)
    {
        // 构建了会话之后，将数据库中的数据加载到内存中
        auto sessions = _dataManager.getAllSessionInfo(); 
        for(const auto& session : sessions)
        {
            _sessions[session->_id] = session;
        }
    }
    // id格式 : session_时间戳_会话计数
    std::string SessionManager::generateSessionId()
    {
        // 获取时间戳
        std::time_t timestamp = time(nullptr);
        // 获取会话计数++
        _sessionCounter.fetch_add(1); // 每次加1
        // 构建会话id
        std::ostringstream os;
        os << "session_" << timestamp << "_" << std::setw(8) << std::setfill('0') << _sessionCounter;
        return os.str();
    }

    // id格式 : message_时间戳_会话计数
    std::string SessionManager::generateMessageId(size_t messageCounter)
    {
        std::time_t timestamp = time(nullptr);
        messageCounter++;
        std::ostringstream os;
        os << "message_" << timestamp << "_" << std::setw(8) << std::setfill('0') << messageCounter;
        return os.str();
    }

    std::string SessionManager::initSession(std::string& model_name)
    {
        // unordered_map进行操着时需要加锁
        _mutex.lock();
        // 生成会话id
        std::string sessionId = generateSessionId();

        // 构建会话信息
        std::shared_ptr<Session> session = std::make_shared<Session>(model_name);

        // 将会话添加到会话列表中
        _sessions[sessionId] = session;
        session->_id = sessionId;
        session->_createAt = std::time(nullptr);
        session->_updateAt = session->_createAt;

        _mutex.unlock();

        // 插入数据库
        _dataManager.insertSession(*session);
        return sessionId;
    }

    std::shared_ptr<Session> SessionManager::getSession(std::string& sessionId)
    {
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end())
        {
            // 这里说明没找到，会话id错误
            WARNING("session is not exists");
            _mutex.unlock();
            return nullptr;
        }

        // 锁内拷贝 shared_ptr，让数据库 IO 在锁外进行
        std::shared_ptr<Session> session = it->second;
        _mutex.unlock();

        auto messages = _dataManager.getAllMessage(sessionId);

        // 回写前重新加锁：_messages 由 _mutex 保护，
        // 无锁写入会与 addMessage / getHistoryMessage 竞争
        _mutex.lock();
        session->_messages = std::move(messages);
        _mutex.unlock();
        return session;
        

    }

    bool SessionManager::addMessage(const std::string& sessionId, const Message& message)
    {
        _mutex.lock();
        // 获取对应的会话之后才能向相应的会话里面添加信息
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end())
        {
            ERROR("session is not exists");
            _mutex.unlock();
            return false;
        }

        // 创建消息
        Message msg(message._role,message._content);
        msg._id = generateMessageId(it->second->_messages.size());
        // 更新消息时间
        msg._timestamp = time(nullptr);
        INFO("addMessage: message create success, messageId : {}, message content : {}",msg._id,msg._content);

        // 插入信息
        it->second->_messages.emplace_back(msg);
        // 更新会话时间
        it->second->_updateAt = time(nullptr);
        _mutex.unlock();
        
        // 将消息插入数据库
        _dataManager.insertMessage(sessionId,msg);
        return true;
        
    }
    // 获取该会话中的所有历史消息
    std::vector<Message> SessionManager::getHistoryMessage(std::string sessionId)
    {
        _mutex.lock();
        // 获取对应的会话
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end())
        {
            ERROR("session is not exists");
            _mutex.unlock();
            return {};
        }
        // 在锁内拷贝消息列表，避免解锁后读取与写入端竞争
        auto messages = it->second->_messages;
        _mutex.unlock();
        return messages;
    }

    std::vector<std::string> SessionManager::getSessionLists()
    {
        _mutex.lock();
        // 获取会话可以将它降序排列
        // 构建一个临时对话列表，将其内容的会话按照会话更新的时间戳降序排列
        std::vector<std::pair<std::time_t,std::shared_ptr<Session>>> temp;
        temp.reserve(_sessions.size());  // 提前开辟空间，减少拷贝
        for(auto& session : _sessions)
        {
            temp.emplace_back(session.second->_updateAt,session.second);
        }
        // 将会话按时间进行排序
        std::sort(temp.begin(),temp.end(),[](auto& a,auto& b){
            return a.first > b.first;
        });

        // 将排序后的会话插入vector中
        std::vector<std::string> sessions;
        sessions.reserve(_sessions.size());
        for(const auto& session : temp)
        {
            sessions.push_back(session.second->_id);
        }
        INFO("getSessionLists: insert vector success");
        _mutex.unlock();
        return sessions;
        
    }

    void SessionManager::deleteSession(const std::string sessionId)
    {
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end())
        {
            ERROR("deleteSession: session is not exists,please input sessionId and try again");
            _mutex.unlock();
            return;
        }
        _sessions.erase(sessionId);
        _mutex.unlock();

        // 删除数据库中的会话
        _dataManager.deleteSession(sessionId);
    }

    void SessionManager::deleteAllSession()
    {
        _mutex.lock();
        _sessions.clear();
        _mutex.unlock();
        // 删除数据库中的所有会话
        _dataManager.deleteAllSession();
    }

    void SessionManager::updateSessionTimestamp(const std::string& sessionId)
    {
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end())
        {
            it->second->_updateAt = time(nullptr);
            _mutex.unlock();

            // 更新数据库中的会话时间戳
            _dataManager.updateSessionTimestamp(sessionId, it->second->_updateAt);
        }
        else
        {
            ERROR("updateSessionTimestamp: session is not exists");
            _mutex.unlock();
        }
    }

    size_t SessionManager::sessionTotal()
    {
        return _sessionCounter;
    }
}