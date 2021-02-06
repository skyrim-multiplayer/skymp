const defaultState = {
  show: false,
  list: [
    {
      id: 0,
      name: 'Лежать',
    },
    {
      id: 1,
      name: 'Сидеть',
    }
  ],
};

export const animListReducer = (state = defaultState, action) => {
  switch (action.type) {
    case "UPDATE_ANIMLIST_SHOW": {
      return {
        ...state,
        show: action.data,
      }
    }
  }
  return state;
}
