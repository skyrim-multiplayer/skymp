import React from 'react';
import ReactDOM from 'react-dom';

import App from './App';

import { store } from './redux/store';
import { Provider } from 'react-redux';

import { Widgets } from './utils/Widgets';

import './main.scss';
import {login} from "./widgets/ExampleWidgets";

if (!window.skyrimPlatform) {
  window.skyrimPlatform = {};
}

if (!window.skyrimPlatform.widgets) {
  window.skyrimPlatform.widgets = new Widgets([]);
}

ReactDOM.render(
  <React.StrictMode>
    <Provider store={store}>
      <App elem={window.skyrimPlatform.widgets.get()} />
    </Provider>
  </React.StrictMode>,
  document.getElementById('root')
);
