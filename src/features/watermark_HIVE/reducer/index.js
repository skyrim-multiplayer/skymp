const defaultState = {
  show: true, // По умолчанию.
  userID: 204, // Демонстративно. Получим со стороны сервера.
  playersOnline: 854 // Демонстративно. Получим (и будем получать) со стороны сервера.
};

export const watermarkHiveReducer = (state = defaultState, action) => {

  switch (action.type) {
    case 'UPDATE_WATERMARK_SHOW': {
      return {
        ...state,
        show: action.data
      }
    }
    case 'UPDATE_WATERMARK_USER_ID': {
      return {
        ...state,
        userID: action.data
      }
    }
    case 'UPDATE_WATERMARK_PLAYERS_ONLINE': {
      return {
        ...state,
        playersOnline: action.data
      }
    }
  }

  return state;

}
