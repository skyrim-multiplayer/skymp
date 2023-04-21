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

let validator = {
  set: function(target, key, value) {
      console.log(`The property ${key} has been updated with ${value}, stack trace: ${(new Error()).stack}`);
      return true;
  }
};
window.skyrimPlatform = new Proxy(window.skyrimPlatform, validator);

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

if (window.skyrimPlatform?.sendMessage) {
  window.skyrimPlatform.sendMessage('front-loaded');
}
