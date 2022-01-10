import React from 'react';
import ReactDOM from 'react-dom';

import App from './App';

import { store } from './redux/store';
import { Provider } from 'react-redux';

import { Widgets } from './utils/Widgets';

import './main.scss';

if (!window.skyrimPlatform) {
  window.skyrimPlatform = {};
}

window.skyrimPlatform.widgets = new Widgets([
  {
    type: 'form',
    id: 2,
    caption: 'Пример диалога',
    elements: [
      { type: 'text', text: 'Содержимое (текст)' },
      { type: 'button', text: 'Кнопка 1' },
      {
        type: 'button',
        text: 'Кнопка 2',
        hint: 'Подсказка',
        tags: ['HINT_STYLE_RIGHT', 'ELEMENT_SAME_LINE'],
        click: () => {
          window.skyrimPlatform.widgets.set([
            {
              type: 'form',
              id: 2,
              caption: 'Пример диалога',
              elements: [
                { type: 'text', text: 'Содержимое (текст)' },
                { type: 'button', text: 'Кнопка 1' },
                {
                  type: 'button',
                  text: 'Кнопка 123123',
                  hint: 'Подсказка',
                  tags: ['HINT_STYLE_RIGHT', 'ELEMENT_SAME_LINE'],
                  click: () => {
                    window.skyrimPlatform.widgets.set();
                  }
                }
              ]
            }, { type: 'chat', messages: ['1', '2', '3'] }]);
        }
      }
    ]
  }, { type: 'chat', messages: ['1', '2'] }]);

ReactDOM.render(
  <React.StrictMode>
    <Provider store={store}>
      <App elem={window.skyrimPlatform.widgets.get()} />
    </Provider>
  </React.StrictMode>,
  document.getElementById('root')
);
