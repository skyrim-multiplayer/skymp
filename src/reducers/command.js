const defaultState = {
}

export const commandReducer = (state = defaultState, action) => {
  switch (action.type) {
    case 'COMMAND': {
      console.log(action.data.commandType, action.data.commandArgs)
      switch (action.data.commandType) {
        case 'SHOW_ANIMLIST': {
          return setTimeout(() => {
            window.storage.dispatch({
              type: 'UPDATE_ANIMLIST_SHOW',
              data: {
                show: true,
                list: action.data.commandArgs.anims
              },
            })
          }, 1)
        }
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

