const defaultState = {
  show: false,
  list: [],
};

export const animListReducer = (state = defaultState, action) => {
  switch (action.type) {
    case "UPDATE_ANIMLIST_SHOW": {
      return {
        ...state,
        show: action.data.show,
        list: action.data.list != null ? action.data.list : [],
      }
    }
  }
  return state;
}
