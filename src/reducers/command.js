const defaultState = {
}

export const commandReducer = (state = defaultState, action) => {
  switch (action.type) {
    case 'COMMAND': {
      switch (action.data.commandType) {
        default:
          setTimeout(() => {
            window.storage.dispatch({
              type: 'ADD_CHAT_MSG',
              data: action.data.alter,
            })
          }, 1)
      }
      return {
        ...state
      }
    }
  }

  return state
}

