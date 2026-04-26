(function () {
  'use strict';

  var MAX_MESSAGE_LENGTH = 300;
  var MAX_RENDERED_MESSAGES = 80;
  var HISTORY_LIMIT = 30;
  var STORAGE_KEY = 'skymp5-chat:visible-history';

  var root = null;
  var log = null;
  var form = null;
  var input = null;
  var widgetRoot = null;
  var commandHistory = [];
  var commandHistoryIndex = 0;
  var pendingLocalEchoes = [];

  function ready(fn) {
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', fn, { once: true });
    } else {
      fn();
    }
  }

  function getElements() {
    root = document.getElementById('skymp-chat');
    log = document.getElementById('chat-log');
    form = document.getElementById('chat-form');
    input = document.getElementById('chat-input');
    widgetRoot = document.getElementById('widget-root');
    return root && log && form && input && widgetRoot;
  }

  function createWidgetStore() {
    var value = [];
    var listeners = [];

    return {
      get: function () {
        return value.slice();
      },
      set: function (next) {
        value = Array.isArray(next) ? next.slice() : [];
        listeners.slice().forEach(function (listener) {
          try {
            listener(value.slice());
          } catch (err) {
            console.error('[skymp5-chat] widget listener failed', err);
          }
        });
      },
      subscribe: function (listener) {
        if (typeof listener !== 'function') return function () {};
        listeners.push(listener);
        return function () {
          listeners = listeners.filter(function (item) {
            return item !== listener;
          });
        };
      },
      addListener: function (listener) {
        this.subscribe(listener);
      },
      removeListener: function (listener) {
        listeners = listeners.filter(function (item) {
          return item !== listener;
        });
      },
    };
  }

  function sendBrowserMessage(payload) {
    if (payload && typeof payload === 'object' && window.skymp && typeof window.skymp.send === 'function') {
      window.skymp.send(payload);
      return true;
    }

    if (
      payload &&
      typeof payload === 'object' &&
      typeof payload.type === 'string' &&
      window.skyrimPlatform &&
      typeof window.skyrimPlatform.sendMessage === 'function'
    ) {
      return window.skyrimPlatform.sendMessage(payload.type, payload.data) !== false;
    }

    if (window.skyrimPlatform && typeof window.skyrimPlatform.sendMessage === 'function') {
      return window.skyrimPlatform.sendMessage(payload) !== false;
    }

    if (window.skymp && typeof window.skymp.send === 'function') {
      window.skymp.send(payload);
      return true;
    }

    console.error('[skymp5-chat] browser message bridge is not available', payload);
    return false;
  }

  function ensureCompatGlobals() {
    window.skyrimPlatform = window.skyrimPlatform || {};
    window.skyrimPlatform.widgets = window.skyrimPlatform.widgets || createWidgetStore();
    window.skyrimPlatform.sendMessage = window.skyrimPlatform.sendMessage || function (payload) {
      if (window.skymp && typeof window.skymp.send === 'function') {
        window.skymp.send(payload);
        return true;
      }
      console.error('[skymp5-chat] window.skyrimPlatform.sendMessage is not available', payload);
      return false;
    };
    window.chatMessages = window.chatMessages || [];

    window.mp = window.mp || {};
    window.mp.send = function (type, data) {
      return sendBrowserMessage({ type: type, data: data });
    };
    window.playSound = window.playSound || function () {};
    window.scrollToLastMessage = scrollToLastMessage;
  }

  function parseColorMessage(raw) {
    var message = String(raw || '');
    var parts = [];
    var color = '#fafafa';
    var cursor = 0;
    var pattern = /#\{([0-9a-fA-F]{6})\}/g;
    var match = null;

    while ((match = pattern.exec(message)) !== null) {
      if (match.index > cursor) {
        parts.push({ text: message.slice(cursor, match.index), color: color });
      }
      color = '#' + match[1];
      cursor = pattern.lastIndex;
    }

    if (cursor < message.length) {
      parts.push({ text: message.slice(cursor), color: color });
    }

    return parts.length ? parts : [{ text: message, color: color }];
  }

  function stripColorMarkers(raw) {
    return String(raw || '').replace(/#\{[0-9a-fA-F]{6}\}/g, '');
  }

  function escapeColorMarkers(text) {
    return String(text || '').replace(/#\{/g, '# {');
  }

  function addMessage(raw) {
    if (!log && !getElements()) return;

    var message = String(raw || '');
    if (!message) return;

    clearMatchingPendingEcho(message);
    renderMessage(message);

    window.chatMessages.push(message);
    while (window.chatMessages.length > MAX_RENDERED_MESSAGES) {
      window.chatMessages.shift();
    }

    saveVisibleHistory();
    scrollToLastMessage();
  }

  function renderMessage(message, extraClass) {
    if (!log && !getElements()) return;

    var line = document.createElement('div');
    line.className = 'chat-line' + (extraClass ? ' ' + extraClass : '');

    parseColorMessage(message).forEach(function (part) {
      if (!part.text) return;
      var span = document.createElement('span');
      span.style.color = part.color;
      span.textContent = part.text;
      line.appendChild(span);
    });

    if (!line.childNodes.length) return;
    log.appendChild(line);

    while (log.childNodes.length > MAX_RENDERED_MESSAGES) {
      log.removeChild(log.firstChild);
    }

    return line;
  }

  function removeHistoryMessage(message) {
    for (var i = window.chatMessages.length - 1; i >= 0; i--) {
      if (window.chatMessages[i] === message) {
        window.chatMessages.splice(i, 1);
        return;
      }
    }
  }

  function clearMatchingPendingEcho(serverMessage) {
    if (!pendingLocalEchoes.length) return;

    var normalizedServer = stripColorMarkers(serverMessage).toLowerCase();
    var now = Date.now();
    pendingLocalEchoes = pendingLocalEchoes.filter(function (echo) {
      var expired = now - echo.time > 8000;
      var matched = !expired && normalizedServer.indexOf(echo.text.toLowerCase()) >= 0;

      if (matched && echo.line && echo.line.parentNode) {
        echo.line.parentNode.removeChild(echo.line);
        removeHistoryMessage(echo.message);
      }

      return !expired && !matched;
    });
  }

  function addLocalEcho(text) {
    if (!log && !getElements()) return;

    var clean = String(text || '').trim();
    if (!clean || clean === '__reload__') return;

    var isSlashInput = clean.charAt(0) === '/';
    var message = isSlashInput
      ? '#{8fb4ff}[Command] #{dbe7ff}' + escapeColorMarkers(clean)
      : '#{aaaaaa}[You] #{ffffff}' + escapeColorMarkers(clean);
    var line = renderMessage(message, 'local-echo');

    window.chatMessages.push(message);
    while (window.chatMessages.length > MAX_RENDERED_MESSAGES) {
      window.chatMessages.shift();
    }

    if (!isSlashInput) {
      pendingLocalEchoes.push({
        text: clean,
        message: message,
        line: line,
        time: Date.now(),
      });
    }

    saveVisibleHistory();
    scrollToLastMessage();
  }

  function saveVisibleHistory() {
    try {
      window.sessionStorage.setItem(STORAGE_KEY, JSON.stringify(window.chatMessages.slice(-MAX_RENDERED_MESSAGES)));
    } catch (err) {
      console.error('[skymp5-chat] failed to save chat history', err);
    }
  }

  function loadVisibleHistory() {
    try {
      var raw = window.sessionStorage.getItem(STORAGE_KEY);
      if (!raw) return;
      var messages = JSON.parse(raw);
      if (!Array.isArray(messages)) return;
      messages.slice(-MAX_RENDERED_MESSAGES).forEach(function (message) {
        if (typeof message === 'string' && message) {
          window.chatMessages.push(message);
          renderMessage(message);
        }
      });
      scrollToLastMessage();
    } catch (err) {
      console.error('[skymp5-chat] failed to load chat history', err);
    }
  }

  function requestServerHistory(attempt) {
    sendToServer('__reload__', { silent: true });

    if ((attempt || 0) < 10) {
      window.setTimeout(function () {
        requestServerHistory((attempt || 0) + 1);
      }, 2000);
    }
  }

  function scrollToLastMessage() {
    if (log) log.scrollTop = log.scrollHeight;
  }

  function openChat(seed) {
    if (!root && !getElements()) return;

    root.classList.add('is-open');
    input.value = seed || '';
    window.setTimeout(function () {
      input.focus();
      input.setSelectionRange(input.value.length, input.value.length);
    }, 0);
  }

  function closeChat() {
    if (!root && !getElements()) return;

    input.value = '';
    input.blur();
    root.classList.remove('is-open');
    commandHistoryIndex = commandHistory.length;
  }

  function rememberInput(text) {
    if (!text) return;
    if (commandHistory[commandHistory.length - 1] !== text) {
      commandHistory.push(text);
      while (commandHistory.length > HISTORY_LIMIT) commandHistory.shift();
    }
    commandHistoryIndex = commandHistory.length;
  }

  function sendToServer(text, options) {
    if (window.mp && typeof window.mp.send === 'function') {
      window.mp.send('cef::chat:send', text);
      return true;
    }

    console.error('[skymp5-chat] window.mp.send is not available');
    if (!options || !options.silent) {
      addMessage('#{ff9933}[Chat] #{ffffff}Not connected to the SkyMP browser bridge.');
    }
    return false;
  }

  function getTags(element) {
    return Array.isArray(element && element.tags) ? element.tags : [];
  }

  function renderWidgets(widgets) {
    if (!widgetRoot && !getElements()) return;

    widgetRoot.innerHTML = '';
    var list = Array.isArray(widgets) ? widgets : [];
    widgetRoot.classList.toggle('has-widgets', list.length > 0);

    list.forEach(function (widget) {
      if (!widget || widget.type !== 'form') return;
      widgetRoot.appendChild(renderFormWidget(widget));
    });
  }

  function renderFormWidget(widget) {
    var container = document.createElement('article');
    container.className = 'ui-form';

    if (widget.caption) {
      var title = document.createElement('header');
      title.className = 'ui-form-title';
      title.textContent = String(widget.caption);
      container.appendChild(title);
    }

    var body = document.createElement('section');
    body.className = 'ui-form-body';

    (Array.isArray(widget.elements) ? widget.elements : []).forEach(function (element, index) {
      var row = document.createElement('div');
      var tags = getTags(element);
      row.className = 'ui-row';
      if (tags.indexOf('ELEMENT_SAME_LINE') >= 0) row.className += ' is-inline';
      if (tags.indexOf('ELEMENT_STYLE_MARGIN_EXTENDED') >= 0) row.className += ' is-extended';

      var node = renderWidgetElement(element, index);
      if (!node) return;

      row.appendChild(node);

      if (element.hint) {
        var hint = document.createElement('div');
        hint.className = 'ui-hint';
        hint.textContent = String(element.hint);
        row.appendChild(hint);
      }

      body.appendChild(row);
    });

    container.appendChild(body);
    return container;
  }

  function renderWidgetElement(element, index) {
    if (!element || !element.type) return null;

    if (element.type === 'text') {
      var text = document.createElement('div');
      text.className = 'ui-text';
      text.textContent = String(element.text || '');
      return text;
    }

    if (element.type === 'icon') {
      var icon = document.createElement('div');
      icon.className = 'ui-icon-label';
      icon.textContent = String(element.text || '');
      return icon;
    }

    if (element.type === 'button') {
      var button = document.createElement('button');
      button.className = 'ui-button';
      button.type = 'button';
      button.disabled = Boolean(element.isDisabled);
      button.textContent = String(element.text || 'Continue');
      button.addEventListener('click', function (event) {
        event.preventDefault();
        event.stopPropagation();
        if (button.disabled || typeof element.click !== 'function') return;
        try {
          element.click();
        } catch (err) {
          console.error('[skymp5-chat] widget button failed', err);
        }
      });
      return button;
    }

    if (element.type === 'inputText' || element.type === 'inputPass') {
      var inputNode = document.createElement('input');
      inputNode.className = 'ui-input';
      inputNode.type = element.type === 'inputPass' ? 'password' : 'text';
      inputNode.name = String(element.name || index);
      inputNode.placeholder = String(element.placeholder || '');
      inputNode.value = String(element.initialValue || '');
      inputNode.disabled = Boolean(element.isDisabled);
      inputNode.addEventListener('input', function () {
        if (typeof element.onInput !== 'function') return;
        element.onInput({ target: inputNode, currentTarget: inputNode });
      });
      return inputNode;
    }

    if (element.type === 'checkBox') {
      var label = document.createElement('label');
      label.className = 'ui-check';

      var checkbox = document.createElement('input');
      checkbox.type = 'checkbox';
      checkbox.checked = Boolean(element.initialValue);
      checkbox.disabled = Boolean(element.isDisabled);
      checkbox.addEventListener('change', function () {
        if (typeof element.setChecked === 'function') element.setChecked(checkbox.checked);
      });

      var checkText = document.createElement('span');
      checkText.textContent = String(element.text || '');

      label.appendChild(checkbox);
      label.appendChild(checkText);
      return label;
    }

    return null;
  }

  function submitChat() {
    if (!input) return;

    var text = String(input.value || '').trim().slice(0, MAX_MESSAGE_LENGTH);
    if (text) {
      rememberInput(text);
      if (sendToServer(text)) addLocalEcho(text);
    }
    closeChat();
  }

  function isTypingTarget(target) {
    return target && (
      target === input ||
      target.tagName === 'INPUT' ||
      target.tagName === 'TEXTAREA' ||
      target.isContentEditable
    );
  }

  function moveHistory(delta) {
    if (!commandHistory.length) return;
    commandHistoryIndex = Math.max(0, Math.min(commandHistory.length, commandHistoryIndex + delta));
    input.value = commandHistory[commandHistoryIndex] || '';
    input.setSelectionRange(input.value.length, input.value.length);
  }

  function bindEvents() {
    form.addEventListener('submit', function (event) {
      event.preventDefault();
      event.stopPropagation();
      submitChat();
    });

    input.addEventListener('keydown', function (event) {
      event.stopPropagation();

      if (event.key === 'Escape') {
        event.preventDefault();
        closeChat();
      } else if (event.key === 'ArrowUp') {
        event.preventDefault();
        moveHistory(-1);
      } else if (event.key === 'ArrowDown') {
        event.preventDefault();
        moveHistory(1);
      }
    }, true);

    input.addEventListener('click', function (event) {
      event.stopPropagation();
    }, true);

    document.addEventListener('keydown', function (event) {
      if (event.defaultPrevented || isTypingTarget(event.target)) return;

      if (event.key === 'Enter' || event.key === '/') {
        event.preventDefault();
        event.stopPropagation();
        openChat(event.key === '/' ? '/' : '');
      }
    }, true);
  }

  function signalLoaded(attempt) {
    if (sendBrowserMessage('front-loaded')) return;
    if ((attempt || 0) < 20) {
      window.setTimeout(function () {
        signalLoaded((attempt || 0) + 1);
      }, 250);
    }
  }

  function init() {
    if (!getElements()) {
      console.error('[skymp5-chat] chat DOM is missing');
      return;
    }

    ensureCompatGlobals();
    window.skyrimPlatform.widgets.subscribe(renderWidgets);
    renderWidgets(window.skyrimPlatform.widgets.get());
    loadVisibleHistory();

    window.skympChat = window.skympChat || {};
    window.skympChat.MAX_MESSAGE_LENGTH = MAX_MESSAGE_LENGTH;
    window.skympChat.addMessage = addMessage;
    window.skympChat.open = openChat;
    window.skympChat.close = closeChat;
    window.skympChat.send = function (text) {
      var value = String(text || '').trim().slice(0, MAX_MESSAGE_LENGTH);
      if (value && sendToServer(value)) addLocalEcho(value);
    };

    bindEvents();
    signalLoaded();
    requestServerHistory(0);
  }

  ready(init);
})();
