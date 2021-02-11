const defaultState = {
  show: false, // По умолчанию.
};

export const tradeHiveReducer = (state = defaultState, action) => {

  switch (action.type) {
    case 'UPDATE_TRADE_SHOW': {
      return {
        ...state,
        show: action.data
      }
    }
  }

  return state;

}
