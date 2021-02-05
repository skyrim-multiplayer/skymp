const defaultState = {
  isBrowserFocus: false,
  // nickname: 'My_Nickname',
  // prefix: 0,
}

export const appReducer = (state = defaultState, action) => {
  switch (action.type) {
    case 'UPDATE_APP_BROWSERFOCUS': {
      return {
        ...state,
        isBrowserFocus: action.data,
      }
    }
  }

  return state
}

