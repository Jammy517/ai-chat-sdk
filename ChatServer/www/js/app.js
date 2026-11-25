/**
 * AI聊天助手 - 页面逻辑
 */
(function () {
  'use strict';

  var MAX_LEN = 2000;

  var els = {
    sessionList: document.getElementById('sessionList'),
    welcome: document.getElementById('welcomeView'),
    chat: document.getElementById('chatView'),
    messages: document.getElementById('messageList'),
    input: document.getElementById('inputBox'),
    counter: document.getElementById('counter'),
    sendBtn: document.getElementById('btnSend'),
    modal: document.getElementById('modelModal'),
    modelGrid: document.getElementById('modelGrid'),
    confirmBtn: document.getElementById('btnConfirm'),
    cancelBtn: document.getElementById('btnCancel'),
    btnNewTop: document.getElementById('btnNewTop'),
    btnNewWelcome: document.getElementById('btnNewWelcome'),
    toast: document.getElementById('toast'),
    serverInfo: document.getElementById('serverInfo'),
    confirmModal: document.getElementById('confirmModal'),
    confirmText: document.getElementById('confirmText'),
    confirmCancelBtn: document.getElementById('btnConfirmCancel'),
    confirmDeleteBtn: document.getElementById('btnConfirmDelete')
  };

  var state = {
    sessions: [],
    models: null,          // 模型列表缓存
    currentId: '',         // 当前会话 id
    pendingModel: '',      // 弹窗中选中的模型
    streaming: false,      // 是否正在接收流式回复
    abort: null,           // 当前流式请求的中止控制器
    pendingDelete: null,   // 等待确认删除的会话
    loadToken: 0           // 防止快速切换会话时的竞态
  };

  var toastTimer = 0;

  /* ==================== 工具函数 ==================== */

  function pad2(n) {
    return n < 10 ? '0' + n : String(n);
  }

  // 消息时间：当天只显示时分，跨天显示日期
  function formatClock(seconds) {
    if (!seconds) return '';
    var date = new Date(seconds * 1000);
    var now = new Date();
    var clock = pad2(date.getHours()) + ':' + pad2(date.getMinutes());
    if (date.toDateString() === now.toDateString()) return clock;
    return date.getFullYear() + '-' + pad2(date.getMonth() + 1) + '-' + pad2(date.getDate()) + ' ' + clock;
  }

  // 会话列表时间：使用相对时间
  function formatRelative(seconds) {
    if (!seconds) return '';
    var time = new Date(seconds * 1000).getTime();
    var diff = Math.max(0, Date.now() - time);
    var MINUTE = 60000;
    var HOUR = 3600000;
    var DAY = 86400000;

    if (diff < MINUTE) return '刚刚';
    if (diff < HOUR) return Math.floor(diff / MINUTE) + ' 分钟前';
    if (diff < DAY) return Math.floor(diff / HOUR) + ' 小时前';
    if (diff < DAY * 7) return Math.floor(diff / DAY) + ' 天前';

    var date = new Date(seconds * 1000);
    return date.getFullYear() + '-' + pad2(date.getMonth() + 1) + '-' + pad2(date.getDate());
  }

  function nowSeconds() {
    return Math.floor(Date.now() / 1000);
  }

  function showToast(message) {
    els.toast.textContent = message;
    els.toast.hidden = false;
    if (toastTimer) clearTimeout(toastTimer);
    toastTimer = setTimeout(function () {
      els.toast.hidden = true;
    }, 2400);
  }

  function showWelcome() {
    els.chat.hidden = true;
    els.welcome.hidden = false;
  }

  function showChat() {
    els.welcome.hidden = true;
    els.chat.hidden = false;
  }

  function findSession(id) {
    for (var i = 0; i < state.sessions.length; i += 1) {
      if (state.sessions[i].id === id) return state.sessions[i];
    }
    return null;
  }

  /* ==================== 会话列表 ==================== */

  function sortSessions(list) {
    return list.slice().sort(function (a, b) {
      return (b.updated_at || 0) - (a.updated_at || 0);
    });
  }

  function refreshSessions() {
    return Api.getSessions().then(function (list) {
      state.sessions = sortSessions(list);
      renderSessions();
    }).catch(function (err) {
      showToast('获取会话列表失败：' + err.message);
    });
  }

  function renderSessions() {
    els.sessionList.innerHTML = '';

    if (!state.sessions.length) {
      var empty = document.createElement('div');
      empty.className = 'session-empty';
      empty.textContent = '暂无历史会话\n点击“新建对话”开始';
      els.sessionList.appendChild(empty);
      return;
    }

    state.sessions.forEach(function (session) {
      els.sessionList.appendChild(buildSessionItem(session));
    });
  }

  function buildSessionItem(session) {
    var item = document.createElement('div');
    item.className = 'session-item' + (session.id === state.currentId ? ' is-active' : '');
    item.dataset.id = session.id;

    var main = document.createElement('div');
    main.className = 'session-main';

    var title = document.createElement('div');
    title.className = 'session-title';
    title.textContent = (session.first_user_message || '').trim() || '新会话';
    title.title = title.textContent;

    var meta = document.createElement('div');
    meta.className = 'session-meta';

    var model = document.createElement('span');
    model.className = 'session-model';
    model.textContent = session.model || '未知模型';
    model.title = model.textContent;

    var time = document.createElement('span');
    time.className = 'session-time';
    time.textContent = formatRelative(session.updated_at || session.created_at);

    meta.appendChild(model);
    meta.appendChild(time);
    main.appendChild(title);
    main.appendChild(meta);

    var del = document.createElement('button');
    del.className = 'session-del';
    del.type = 'button';
    del.textContent = '...';
    del.title = '删除该会话';
    del.addEventListener('click', function (event) {
      event.stopPropagation();
      removeSession(session);
    });

    item.appendChild(main);
    item.appendChild(del);
    item.addEventListener('click', function () {
      openSession(session.id);
    });

    return item;
  }

  /* ==================== 会话操作 ==================== */

  function openSession(sessionId) {
    if (state.streaming) {
      showToast('请等待当前回复完成');
      return;
    }
    if (!sessionId) return;

    state.currentId = sessionId;
    state.loadToken += 1;
    var token = state.loadToken;

    showChat();
    renderSessions();
    els.messages.innerHTML = '<div class="chat-empty">正在加载历史消息...</div>';

    Api.getHistory(sessionId).then(function (messages) {
      if (token !== state.loadToken) return;
      renderHistory(messages);
    }).catch(function (err) {
      if (token !== state.loadToken) return;
      els.messages.innerHTML = '';
      showToast('获取历史消息失败：' + err.message);
    });
  }

  function renderHistory(messages) {
    els.messages.innerHTML = '';

    if (!messages.length) {
      els.messages.innerHTML = '<div class="chat-empty">该会话还没有消息，开始提问吧</div>';
      return;
    }

    messages.forEach(function (message) {
      els.messages.appendChild(buildMessage(message.role, message.content, message.timestamp));
    });

    scrollToBottom();
  }

  // 点击“...”先弹出警告确认，确认后才真正删除
  function removeSession(session) {
    if (state.streaming && session.id === state.currentId) {
      showToast('请等待当前回复完成');
      return;
    }

    state.pendingDelete = session;

    var title = (session.first_user_message || '').trim() || '新会话';
    els.confirmText.textContent =
      '删除后「' + title + '」及其全部聊天记录将无法恢复，确定要删除吗？';
    els.confirmDeleteBtn.disabled = false;
    els.confirmDeleteBtn.textContent = '删除';
    els.confirmModal.hidden = false;
  }

  function closeConfirm() {
    els.confirmModal.hidden = true;
    state.pendingDelete = null;
    els.confirmDeleteBtn.disabled = false;
    els.confirmDeleteBtn.textContent = '删除';
  }

  function confirmDelete() {
    var session = state.pendingDelete;
    if (!session) return;

    els.confirmDeleteBtn.disabled = true;
    els.confirmDeleteBtn.textContent = '删除中...';

    Api.deleteSession(session.id).then(function () {
      state.sessions = state.sessions.filter(function (item) {
        return item.id !== session.id;
      });

      if (state.currentId === session.id) {
        state.currentId = '';
        els.messages.innerHTML = '';
        showWelcome();
      }

      renderSessions();
      closeConfirm();
      showToast('会话已删除');
    }).catch(function (err) {
      els.confirmDeleteBtn.disabled = false;
      els.confirmDeleteBtn.textContent = '删除';
      showToast('删除失败：' + err.message);
    });
  }

  /* ==================== 消息渲染 ==================== */

  function buildMessage(role, content, timestamp) {
    var isUser = role === 'user';

    var wrap = document.createElement('div');
    wrap.className = 'msg ' + (isUser ? 'msg-user' : 'msg-assistant');

    var bubble = document.createElement('div');
    bubble.className = 'bubble' + (isUser ? '' : ' markdown');

    if (isUser) {
      bubble.textContent = content;
    } else {
      bubble.innerHTML = Markdown.render(content);
    }

    var time = document.createElement('div');
    time.className = 'msg-time';
    time.textContent = formatClock(timestamp || nowSeconds());

    wrap.appendChild(bubble);
    wrap.appendChild(time);
    return wrap;
  }

  function buildStreamingMessage() {
    var wrap = document.createElement('div');
    wrap.className = 'msg msg-assistant';

    var bubble = document.createElement('div');
    bubble.className = 'bubble markdown';
    bubble.innerHTML = '<span class="typing"><i></i><i></i><i></i></span>';

    var time = document.createElement('div');
    time.className = 'msg-time';
    time.textContent = formatClock(nowSeconds());

    wrap.appendChild(bubble);
    wrap.appendChild(time);

    return { wrap: wrap, bubble: bubble, time: time, raw: '' };
  }

  // 消息区始终滚动到底部（新消息、历史加载、流式生成过程中）
  function scrollToBottom() {
    els.messages.scrollTop = els.messages.scrollHeight;
  }

  /* ==================== 发送消息 ==================== */

  function setSending(sending) {
    state.streaming = sending;
    els.sendBtn.classList.toggle('is-stop', sending);
    els.sendBtn.disabled = false;
    els.sendBtn.textContent = sending ? '停止' : '发送';
    els.sendBtn.title = sending ? '点击终止本次回复（Esc）' : '';
  }

  // 一键停止：中止当前流式请求，已生成的内容保留
  function stopGeneration() {
    if (!state.streaming) return;
    if (state.abort) state.abort.abort();
  }

  function sendMessage() {
    if (state.streaming) return;

    var text = els.input.value.trim();
    if (!text) return;

    if (!state.currentId) {
      showToast('请先新建对话或选择一个历史会话');
      return;
    }
    if (text.length > MAX_LEN) {
      showToast('消息长度不能超过 ' + MAX_LEN + ' 字');
      return;
    }

    var placeholder = els.messages.querySelector('.chat-empty');
    if (placeholder) els.messages.innerHTML = '';

    els.input.value = '';
    updateCounter();
    autoGrow();

    els.messages.appendChild(buildMessage('user', text, nowSeconds()));

    var target = buildStreamingMessage();
    els.messages.appendChild(target.wrap);
    scrollToBottom();

    setSending(true);

    state.abort = new AbortController();

    var paintTimer = 0;

    function paint() {
      paintTimer = 0;
      if (!target.raw) return;
      target.bubble.innerHTML = Markdown.render(target.raw) + '<span class="caret"></span>';
      // 生成过程中始终跟随到底部
      scrollToBottom();
    }

    // 合并高频分片：最多每 60ms 重新渲染一次，避免长文本流式输出时界面卡顿
    function schedulePaint() {
      if (!paintTimer) paintTimer = setTimeout(paint, 60);
    }

    function stopPaint() {
      if (paintTimer) clearTimeout(paintTimer);
      paintTimer = 0;
    }

    Api.streamMessage(state.currentId, text, function (delta) {
      if (!delta) return;
      target.raw += delta;
      schedulePaint();
    }, state.abort.signal).then(function () {
      stopPaint();

      if (target.raw) {
        target.bubble.innerHTML = Markdown.render(target.raw);
      } else {
        target.bubble.innerHTML = '<span class="msg-empty">（模型未返回内容）</span>';
      }
      target.time.textContent = formatClock(nowSeconds());
      scrollToBottom();
    }).catch(function (err) {
      stopPaint();

      var aborted = !!err && err.name === 'AbortError';

      if (target.raw) {
        target.bubble.innerHTML = Markdown.render(target.raw);
      } else {
        target.bubble.innerHTML = '';
      }

      var tip = document.createElement('div');
      tip.className = aborted ? 'msg-stopped' : 'msg-error';
      tip.textContent = aborted
        ? '已终止本次回复'
        : ('回复失败：' + (err && err.message ? err.message : '未知错误'));
      target.bubble.appendChild(tip);
      target.time.textContent = formatClock(nowSeconds());
      scrollToBottom();
    }).then(function () {
      state.abort = null;
      setSending(false);
      refreshSessions();
    });
  }

  /* ==================== 新建会话（模型选择弹窗） ==================== */

  /* 每次点击“新建对话”都向服务器获取最新的模型列表（需求 9） */
  function openModelPicker() {
    if (state.streaming) {
      showToast('请等待当前回复完成');
      return;
    }

    state.pendingModel = '';
    els.confirmBtn.disabled = true;
    els.confirmBtn.textContent = '确定';
    els.modelGrid.innerHTML = '<div class="model-loading">正在获取模型列表...</div>';
    els.modal.hidden = false;

    Api.getModels().then(function (models) {
      state.models = models;

      if (!models.length) {
        els.modelGrid.innerHTML = '<div class="model-loading">服务器暂无可用的模型</div>';
        return;
      }

      renderModelGrid(models);
    }).catch(function (err) {
      els.modelGrid.innerHTML = '<div class="model-loading">获取模型列表失败，请稍后重试</div>';
      showToast('获取模型列表失败：' + err.message);
    });
  }

  function renderModelGrid(models) {
    els.modelGrid.innerHTML = '';
    state.pendingModel = '';
    els.confirmBtn.disabled = true;

    models.forEach(function (model) {
      var card = document.createElement('div');
      card.className = 'model-card';
      card.dataset.name = model.name;
      card.setAttribute('role', 'radio');
      card.setAttribute('aria-checked', 'false');
      card.tabIndex = 0;

      var radio = document.createElement('input');
      radio.type = 'radio';
      radio.name = 'chat-model';
      radio.value = model.name;

      var dot = document.createElement('span');
      dot.className = 'radio-dot';

      var name = document.createElement('span');
      name.className = 'model-name';
      name.textContent = model.name;

      var desc = document.createElement('span');
      desc.className = 'model-desc';
      desc.textContent = model.desc || '暂无描述';

      card.appendChild(radio);
      card.appendChild(dot);
      card.appendChild(name);
      card.appendChild(desc);

      card.addEventListener('click', function () {
        selectModel(model.name);
      });
      card.addEventListener('keydown', function (event) {
        if (event.key === 'Enter' || event.key === ' ') {
          event.preventDefault();
          selectModel(model.name);
        }
      });

      els.modelGrid.appendChild(card);
    });
  }

  function selectModel(name) {
    state.pendingModel = name;

    var cards = els.modelGrid.querySelectorAll('.model-card');
    for (var i = 0; i < cards.length; i += 1) {
      var selected = cards[i].dataset.name === name;
      cards[i].classList.toggle('is-selected', selected);
      cards[i].setAttribute('aria-checked', selected ? 'true' : 'false');
      var radio = cards[i].querySelector('input');
      if (radio) radio.checked = selected;
    }

    els.confirmBtn.disabled = !name;
  }

  function closeModal() {
    els.modal.hidden = true;
    state.pendingModel = '';
    els.confirmBtn.disabled = true;
    els.confirmBtn.textContent = '确定';
  }

  function confirmCreate() {
    var model = state.pendingModel;
    if (!model) return;

    els.confirmBtn.disabled = true;
    els.confirmBtn.textContent = '创建中...';

    Api.createSession(model).then(function (data) {
      closeModal();

      if (!data.session_id) throw new Error('服务器未返回会话 id');

      state.currentId = data.session_id;
      state.loadToken += 1;

      state.sessions.unshift({
        id: data.session_id,
        model: data.model || model,
        created_at: nowSeconds(),
        updated_at: nowSeconds(),
        message_count: 0,
        first_user_message: ''
      });

      renderSessions();
      showChat();
      els.messages.innerHTML = '<div class="chat-empty">新的对话已创建，开始提问吧</div>';
      els.input.focus();

      refreshSessions();
    }).catch(function (err) {
      els.confirmBtn.disabled = false;
      els.confirmBtn.textContent = '确定';
      showToast('创建会话失败：' + err.message);
    });
  }

  /* ==================== 输入框 ==================== */

  function updateCounter() {
    var length = els.input.value.length;
    els.counter.textContent = length + '/' + MAX_LEN;
    els.counter.classList.toggle('is-limit', length >= MAX_LEN);
  }

  function autoGrow() {
    els.input.style.height = 'auto';
    els.input.style.height = Math.min(els.input.scrollHeight, 150) + 'px';
  }

  /* ==================== 代码块复制 ==================== */

  function fallbackCopy(text) {
    var area = document.createElement('textarea');
    area.value = text;
    area.setAttribute('readonly', '');
    area.style.position = 'fixed';
    area.style.top = '-9999px';
    document.body.appendChild(area);
    area.select();

    var ok = false;
    try {
      ok = document.execCommand('copy');
    } catch (e) {
      ok = false;
    }
    document.body.removeChild(area);
    return ok;
  }

  function copyText(text) {
    // http://192.168.x.x 不是安全上下文，navigator.clipboard 不可用，需要降级
    if (navigator.clipboard && window.isSecureContext) {
      return navigator.clipboard.writeText(text).then(function () {
        return true;
      }, function () {
        return fallbackCopy(text);
      });
    }
    return Promise.resolve(fallbackCopy(text));
  }

  function handleCopyClick(event) {
    var target = event.target;
    if (!target || !target.closest) return;

    var button = target.closest('.code-copy');
    if (!button) return;

    var block = button.closest('.code-block');
    var code = block ? block.querySelector('code') : null;
    if (!code) return;

    copyText(code.textContent).then(function (ok) {
      button.textContent = ok ? '已复制' : '复制失败';
      setTimeout(function () {
        button.textContent = '复制';
      }, 1500);
    });
  }

  /* ==================== 事件绑定 ==================== */

  function bindEvents() {
    els.btnNewTop.addEventListener('click', openModelPicker);
    els.btnNewWelcome.addEventListener('click', openModelPicker);
    els.sendBtn.addEventListener('click', function () {
      if (state.streaming) stopGeneration();
      else sendMessage();
    });

    els.input.addEventListener('input', function () {
      updateCounter();
      autoGrow();
    });

    var composing = false;
    els.input.addEventListener('compositionstart', function () { composing = true; });
    els.input.addEventListener('compositionend', function () { composing = false; });

    els.input.addEventListener('keydown', function (event) {
      if (event.key === 'Enter' && !event.shiftKey && !composing && !event.isComposing) {
        event.preventDefault();
        sendMessage();
      }
    });

    els.confirmBtn.addEventListener('click', confirmCreate);
    els.cancelBtn.addEventListener('click', closeModal);

    els.modal.addEventListener('click', function (event) {
      if (event.target === els.modal) closeModal();
    });

    els.confirmCancelBtn.addEventListener('click', closeConfirm);
    els.confirmDeleteBtn.addEventListener('click', confirmDelete);
    els.confirmModal.addEventListener('click', function (event) {
      if (event.target === els.confirmModal) closeConfirm();
    });

    document.addEventListener('keydown', function (event) {
      if (event.key !== 'Escape') return;
      if (!els.confirmModal.hidden) {
        closeConfirm();
        return;
      }
      if (!els.modal.hidden) {
        closeModal();
        return;
      }
      // 生成过程中按 Esc 一键终止
      stopGeneration();
    });

    els.messages.addEventListener('click', handleCopyClick);

    els.modelGrid.addEventListener('keydown', function (event) {
      if (event.key === 'Escape') closeModal();
    });
  }

  /* ==================== 初始化 ==================== */

  function init() {
    els.serverInfo.textContent = location.protocol.indexOf('http') === 0
      ? location.host
      : Api.base.replace('http://', '');

    bindEvents();
    updateCounter();
    autoGrow();
    refreshSessions();
  }

  init();
})();
