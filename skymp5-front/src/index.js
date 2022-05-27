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
  window.skyrimPlatform.widgets = new Widgets([
  //   {
  //     type: "chat",
  //     id: 1,
  //     messages: ["Vlad: ((Hello world))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))", "Ярл Воровка: Всем добрый вечер! ((Сейчас покушаю и вернусь))"]
  // }
  ]);
}

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
  const _list = document.querySelector('#chat > .list');
  if (_list != null && window.needToScroll) { _list.scrollTop = _list.offsetHeight * _list.offsetHeight; }
};

if (window.skyrimPlatform?.sendMessage) {
  window.skyrimPlatform.sendMessage('front-loaded');
}
