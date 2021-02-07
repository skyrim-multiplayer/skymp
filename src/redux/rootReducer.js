import { combineReducers } from 'redux'

import { appReducer } from '../reducers/app'
import { commandReducer } from '../reducers/command'
import { chatReducer } from '../features/chat/reducer'
import { animListReducer } from '../features/animList/reducer'
import { chatHiveReducer } from '../features/chat_HIVE/reducer'

export const rootReducer = combineReducers({
  chatHiveReducer,
  commandReducer,
  appReducer,
  chatReducer,
  animListReducer,
})
