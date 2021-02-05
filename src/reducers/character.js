const defaultState = {
  nickname: 'My_Nickname',
  prefix: 0,
}

export const characterReducer = (state = defaultState, action) => {
  switch (action.type) {
    case 'UPDATE_CHARACTER_NICKNAME': {
      return {
        ...state,
        nickname: action.data,
      }
    }
    case 'UPDATE_CHARACTER_PREFIX': {
      return {
        ...state,
        prefix: action.data,
      }
    }
  }

  return state
}
