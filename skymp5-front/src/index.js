import React from 'react'
import ReactDOM from 'react-dom'

import App from './App'

import { store } from './redux/store'
import { Provider } from 'react-redux'

import './main.sass'

ReactDOM.render(
  <React.StrictMode>
    <Provider store={store}>
      <App />
    </Provider>
  </React.StrictMode>,
  document.getElementById('root')
)
