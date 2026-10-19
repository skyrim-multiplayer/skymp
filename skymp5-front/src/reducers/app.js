const defaultState = {
  isBrowserFocus: false,
};

export const appReducer = (state = defaultState, action) => {
  switch (action.type) {
    case 'UPDATE_APP_BROWSERFOCUS': {
      return {
        ...state,
        isBrowserFocus: action.data,
      };
    }
  }

  return state;
};
