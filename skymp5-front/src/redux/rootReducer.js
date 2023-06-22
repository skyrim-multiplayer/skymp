import { combineReducers } from 'redux'

import { appReducer } from '../reducers/app'
import { commandReducer } from '../reducers/command'
import { chatReducer } from '../features/chat/reducer'
import { animListReducer } from '../features/animList/reducer'

export const rootReducer = combineReducers({
  commandReducer,
  appReducer,
  chatReducer,
  animListReducer,
})
