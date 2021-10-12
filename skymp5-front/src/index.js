import React from 'react'
import ReactDOM from 'react-dom'

import App from './App'

import { store } from './redux/store'
import { Provider } from 'react-redux'

import './main.sass'

const msgbox = {
  type: "form",
  id: 2,
  caption: "Пример диалога",
  elements: [
      { type: "text", text: "Содержимое (текст)" },
      { type: "button", text: "Кнопка 1" },
      { type: "button", text: "Кнопка 2", tags: ["ELEMENT_SAME_LINE"] }
  ]
}

const inputText = {
  type: "form",
  id: 2,
  caption: "Пример диалога",
  elements: [
      { type: "text", text: "Содержимое (текст)" },
      { type: "inputText", placeHolder: "подсказка" },
      { type: "button", text: "Кнопка 1", hint: "НАЖМИ МЕНЯ" },
      { type: "button", text: "Кнопка 2",  hint: "BEBRA", tags: ["ELEMENT_SAME_LINE"] }
  ]
}

const login = {
  type: "form",
  id: 1,
  caption: "login",
  elements: [
      { type: "button", tags: ["BUTTON_STYLE_GITHUB"] },
      { type: "button", tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE"] },
      { type: "icon", text: "email", tags: ["ICON_STYLE_MAIL"] },
      { type: "inputText", tags: ["ELEMENT_SAME_LINE"], placeholder: "dude33@gmail.com" },
      { type: "icon", text: "password",  tags: ["ICON_STYLE_KEY"] },
      { type: "inputPass", tags: ["ELEMENT_SAME_LINE"], placeholder: "password, you know" },
      { type: "checkBox", text: "remember me" },
      { type: "button", text: "register now", tags: ["ELEMENT_SAME_LINE", "BUTTON_STYLE_FRAME"]},
      { type: "button", text: "travel to skyrim"},
  ]
};
const loginEmptyCaption = {
  type: "form",
  id: 1,
  elements: [
      { type: "button", tags: ["BUTTON_STYLE_GITHUB"] },
      { type: "button", tags: ["BUTTON_STYLE_PATREON", "ELEMENT_SAME_LINE"] },
      { type: "icon", text: "email", tags: ["ICON_STYLE_MAIL"] },
      { type: "inputText", tags: ["ELEMENT_SAME_LINE"], placeholder: "dude33@gmail.com" },
      { type: "icon", text: "password",  tags: ["ICON_STYLE_KEY"] },
      { type: "inputPass", tags: ["ELEMENT_SAME_LINE"], placeholder: "password, you know" },
      { type: "checkBox", text: "remember me" },
      { type: "button", text: "register now", tags: ["ELEMENT_SAME_LINE", "BUTTON_STYLE_FRAME"]},
      { type: "button", text: "travel to skyrim"},
  ]
};

ReactDOM.render(
  <React.StrictMode>
    <Provider store={store}>
      <App elem={login} />
    </Provider>
  </React.StrictMode>,
  document.getElementById('root')
)
