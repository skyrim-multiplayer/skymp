import { combineReducers } from 'redux'

import { chatReducer } from '../features/chat/reducer'
import { characterReducer } from '../reducers/character'

export const rootReducer = combineReducers({
  chatReducer,
  characterReducer,
})
