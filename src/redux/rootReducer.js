import { combineReducers } from 'redux'

import { chatReducer } from '../features/chat/reducer'
import { appReducer } from '../reducers/app'

export const rootReducer = combineReducers({
  chatReducer,
  appReducer,
})
