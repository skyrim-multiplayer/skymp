import { combineReducers } from 'redux'

import { appReducer } from '../reducers/app'
import { commandReducer } from '../reducers/command'
import { chatReducer } from '../features/chat/reducer'
import { animListReducer } from '../features/animList/reducer'
import { chatHiveReducer } from '../features/chat_HIVE/reducer'
import { animListHiveReducer } from '../features/animList_HIVE/reducer'
import { spawnHiveReducer } from '../features/spawn_HIVE/reducer'
import { watermarkHiveReducer } from '../features/watermark_HIVE/reducer'
import { tradeHiveReducer } from '../features/trade_HIVE/reducer'

export const rootReducer = combineReducers({
  tradeHiveReducer,
  watermarkHiveReducer,
  spawnHiveReducer,
  animListHiveReducer,
  chatHiveReducer,
  commandReducer,
  appReducer,
  chatReducer,
  animListReducer,
})
