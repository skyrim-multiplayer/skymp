import React from 'react';
import ReactDOM from 'react-dom';

import App from './App';

import { store } from './redux/store';
import { Provider } from 'react-redux';

import { Widgets } from './utils/Widgets';

import './main.sass';

if (!window.skyrimPlatform) {
  window.skyrimPlatform = {};
}

const mychat = {
  type: 'chat',
  messages: ['message))', '#{676869}mess#{AAAAAA}age2'],
  send: (message) => {
    console.log('sended ' + message);
  }
};
window.skyrimPlatform = {};

const login = {
  caption: 'Test?',
  type: 'form',
  elements: [
    {
      type: 'button',
      tags: ['BUTTON_STYLE_FRAME'],
      click: () => {
        console.log('Yes');
      }
    },
    {
      type: 'button',
      tags: ['BUTTON_STYLE_FRAME_LEFT'],
      text: 'Sample text 2',
      click: () => {
        window.skyrimPlatform.widgets.set([
          {
            type: 'form',
            id: 1,
            caption: 'login',
            elements: [
              { type: 'button', tags: ['BUTTON_STYLE_GITHUB'] },
              {
                type: 'button',
                tags: ['BUTTON_STYLE_PATREON', 'ELEMENT_SAME_LINE']
              },
              { type: 'icon', text: 'email', tags: ['ICON_STYLE_MAIL'] },
              {
                type: 'inputText',
                tags: ['ELEMENT_SAME_LINE'],
                placeholder: 'dude33@gmail.com'
              },
              { type: 'icon', text: 'password', tags: ['ICON_STYLE_KEY'] },
              {
                type: 'inputPass',
                tags: ['ELEMENT_SAME_LINE'],
                placeholder: 'password, you know'
              },
              { type: 'checkBox', text: 'remember me' },
              {
                type: 'button',
                text: 'register now',
                tags: ['ELEMENT_SAME_LINE', 'BUTTON_STYLE_FRAME'],
                click: (e) => {
                  window.skyrimPlatform.widgets.set([]);
                }
              },
              {
                type: 'button',
                text: 'travel to skyrim',
                tags: ['BUTTON_STYLE_FRAME']
              }
            ]
          }
        ]);
      }
    },
    {
      type: 'button',
      tags: ['BUTTON_STYLE_FRAME_RIGHT'],
      text: 'Sample text 3',
      click: () => {
        console.log('No');
      }
    },
    { type: 'inputText' }
  ]
};

window.skyrimPlatform.widgets = new Widgets([
  login
]);

ReactDOM.render(
  <React.StrictMode>
    <Provider store={store}>
      <App elem={window.skyrimPlatform.widgets.get()} />
    </Provider>
  </React.StrictMode>,
  document.getElementById('root')
);
