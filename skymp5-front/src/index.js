import React from 'react';
import ReactDOM from 'react-dom';

import App from './App';

import { store } from './redux/store';
import { Provider } from 'react-redux';

import { Widgets } from './utils/Widgets';

import './main.scss';

if (!window.skyrimPlatform) {
  window.skyrimPlatform = {};
  window.needToScroll = true;
}

if (!window.skyrimPlatform.widgets) {
  window.skyrimPlatform.widgets = new Widgets([]);
}

window.skympChat = window.skympChat || {};
window.skympChat.MAX_MESSAGE_LENGTH = 300;
window.skympChat.addMessage = (rawMessage) => {
  try {
    const raw = String(rawMessage || '');
    if (!raw) return;

    if (!window.chatMessages) window.chatMessages = [];

    const segments = [];
    let remaining = raw;
    let color = '#fafafa';
    const colorPattern = /^#\{([0-9a-fA-F]{6})\}/;

    while (remaining.length > 0) {
      const colorMatch = remaining.match(colorPattern);
      if (colorMatch) {
        color = `#${colorMatch[1]}`;
        remaining = remaining.slice(colorMatch[0].length);
        continue;
      }

      const nextColorAt = remaining.indexOf('#{');
      if (nextColorAt === 0) {
        segments.push({ text: remaining.slice(0, 2), color, opacity: 1, type: ['default'] });
        remaining = remaining.slice(2);
        continue;
      }

      if (nextColorAt < 0) {
        segments.push({ text: remaining, color, opacity: 1, type: ['default'] });
        break;
      }

      if (nextColorAt > 0) {
        segments.push({ text: remaining.slice(0, nextColorAt), color, opacity: 1, type: ['default'] });
      }
      remaining = remaining.slice(nextColorAt);
    }

    if (!segments.length) return;

    window.chatMessages.push({ text: segments, category: 'default', opacity: 1 });
    while (window.chatMessages.length > 50) window.chatMessages.shift();

    const widgets = window.skyrimPlatform.widgets.get();
    let found = false;
    const nextWidgets = widgets.map((widget) => {
      if (widget.type !== 'chat') return widget;
      found = true;
      return { ...widget, messages: window.chatMessages.slice() };
    });

    if (!found) {
      nextWidgets.push({
        type: 'chat',
        messages: window.chatMessages.slice(),
        send: (message) => window.mp && window.mp.send('cef::chat:send', message)
      });
    }

    window.skyrimPlatform.widgets.set(nextWidgets);
    window.needToScroll = true;
    if (typeof window.scrollToLastMessage === 'function') window.scrollToLastMessage();
  } catch (err) {
    console.error('[chat] Failed to add chat message', err);
  }
};

ReactDOM.render(
  <React.StrictMode>
    <Provider store={store}>
      <App elem={window.skyrimPlatform.widgets.get()} />
    </Provider>
  </React.StrictMode>,
  document.getElementById('root')
);

// Called from skymp5-functions-lib, chatProperty.ts
window.scrollToLastMessage = () => {
  const _list = document.querySelector('#chat > .chat-main > .list > .chat-list');
  if (_list != null && window.needToScroll) { _list.scrollTop = _list.offsetHeight * _list.offsetHeight; }
};

window.playSound = (name) => {
  (new Audio(require('./sound/' + name).default)).play();
};

if (window.skyrimPlatform?.sendMessage) {
  window.skyrimPlatform.sendMessage('front-loaded');
}
