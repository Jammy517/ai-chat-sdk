/**
 * 与 AIChatServer 交互的接口封装
 * 服务器地址：192.168.159.128:8080
 */
(function (global) {
  'use strict';

  /* ===== 服务器地址（部署到其他机器时只需改这两行） ===== */
  var SERVER_HOST = '192.168.159.128';
  var SERVER_PORT = 8080;
  var FALLBACK_SERVER = 'http://' + SERVER_HOST + ':' + SERVER_PORT;

  // 页面本身由该服务器托管时使用相对路径（同源，无跨域问题）；
  // 直接双击打开 index.html（file://）时回退到上面的固定地址
  var BASE = (location.protocol === 'http:' || location.protocol === 'https:')
    ? ''
    : FALLBACK_SERVER;

  function buildUrl(path) {
    return BASE + path;
  }

  function requestJson(path, options) {
    return fetch(buildUrl(path), options).then(function (res) {
      return res.text().then(function (text) {
        var json = null;
        if (text) {
          try {
            json = JSON.parse(text);
          } catch (e) {
            throw new Error('响应解析失败');
          }
        }
        if (!res.ok) {
          throw new Error((json && json.message) || ('请求失败（' + res.status + '）'));
        }
        if (json && json.success === false) {
          throw new Error(json.message || '请求失败');
        }
        return json || {};
      });
    });
  }

  /* 服务端用 Json::valueToQuotedString 包装每个分片，正常是 JSON 字符串字面量，
     这里做一次兼容：能解析成字符串就解析，否则按纯文本处理 */
  function decodePayload(payload) {
    if (payload.charAt(0) === '"') {
      try {
        var value = JSON.parse(payload);
        if (typeof value === 'string') return value;
      } catch (e) {
        /* 忽略，按纯文本返回 */
      }
    }
    return payload;
  }

  var Api = {
    base: BASE,

    /* 1. 获取所有会话列表 */
    getSessions: function () {
      return requestJson('/api/sessions').then(function (res) {
        return Array.isArray(res.data) ? res.data : [];
      });
    },

    /* 2. 获取可用模型（过滤掉服务端可能注册的空模型名） */
    getModels: function () {
      return requestJson('/api/models').then(function (res) {
        var list = Array.isArray(res.data) ? res.data : [];
        return list.filter(function (item) {
          return item && item.name;
        });
      });
    },

    /* 3. 创建新会话 */
    createSession: function (model) {
      return requestJson('/api/session', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ model: model })
      }).then(function (res) {
        return res.data || {};
      });
    },

    /* 4. 删除会话 */
    deleteSession: function (sessionId) {
      return requestJson('/api/session/' + encodeURIComponent(sessionId), {
        method: 'DELETE'
      });
    },

    /* 5. 获取会话历史消息 */
    getHistory: function (sessionId) {
      return requestJson('/api/session/' + encodeURIComponent(sessionId) + '/history')
        .then(function (res) {
          return Array.isArray(res.data) ? res.data : [];
        });
    },

    /* 6. 发送消息（流式）
       onDelta(text) 每收到一段正文回调一次；收到 data: [DONE] 后正常返回
       signal 用于中止请求（一键停止） */
    streamMessage: function (sessionId, message, onDelta, signal) {
      return fetch(buildUrl('/api/message/async'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ session_id: sessionId, message: message }),
        signal: signal
      }).then(function (res) {
        if (!res.ok) {
          return res.text().then(function (text) {
            var msg = text;
            try {
              var json = JSON.parse(text);
              if (json && json.message) msg = json.message;
            } catch (e) { /* 保留原文 */ }
            throw new Error(msg || ('发送消息失败（' + res.status + '）'));
          });
        }
        if (!res.body) {
          throw new Error('当前浏览器不支持流式响应');
        }
        return readSseStream(res.body, onDelta);
      });
    }
  };

  /* 逐块读取并解析 SSE：每段以空行 \n\n 分隔 */
  function readSseStream(body, onDelta) {
    var reader = body.getReader();
    var decoder = new TextDecoder('utf-8');
    var buffer = '';
    var finished = false;

    function handleBlock(block) {
      var lines = block.split('\n');
      for (var i = 0; i < lines.length; i++) {
        var line = lines[i];
        if (line.indexOf('data:') !== 0) continue;

        var payload = line.slice(5);
        if (payload.charAt(0) === ' ') payload = payload.slice(1);
        if (payload === '[DONE]') {
          finished = true;
          return;
        }
        onDelta(decodePayload(payload));
      }
    }

    function pump() {
      return reader.read().then(function (chunk) {
        if (chunk.done) {
          buffer += decoder.decode();
          if (!finished && buffer.trim()) handleBlock(buffer);
          return;
        }
        buffer += decoder.decode(chunk.value, { stream: true });

        var index;
        while ((index = buffer.indexOf('\n\n')) !== -1) {
          var block = buffer.slice(0, index);
          buffer = buffer.slice(index + 2);
          handleBlock(block);
        }
        return pump();
      });
    }

    return pump();
  }

  global.Api = Api;
})(window);
