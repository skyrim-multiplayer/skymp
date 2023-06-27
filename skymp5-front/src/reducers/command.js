const defaultState = {};

export const commandReducer = (state = defaultState, action) => {
  switch (action.type) {
    case 'COMMAND': {
      console.log(action.data.commandType, action.data.commandArgs);
      switch (action.data.commandType) {
        case 'SHOW_ANIMLIST': {
          return setTimeout(() => {
            window.storage.dispatch({
              type: 'UPDATE_ANIMLIST_SHOW',
              data: {
                show: true,
                list: action.data.commandArgs.anims,
              },
            });
          }, 1);
        }
        default:
          setTimeout(() => {
            if (!Array.isArray(action.data.alter)) {
              action.data.alter = [action.data.alter];
            }
            for (const msg of action.data.alter) {
              window.storage.dispatch({
                type: 'ADD_CHAT_MSG',
                data: msg,
              });
            }
          }, 1);
      }
      return {
        ...state,
      };
    }
  }

  return state;
};
