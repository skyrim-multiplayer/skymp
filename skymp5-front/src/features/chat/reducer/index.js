const defaultState = {
  show: true,
  list: [],
  showInput: 'auto',
  input: '',
};

export const chatReducer = (state = defaultState, action) => {
  switch (action.type) {
    case 'UPDATE_CHAT_SHOW': {
      return {
        ...state,
        show: action.data,
      };
    }

    case 'UPDATE_CHAT_LIST': {
      return {
        ...state,
        list: action.data,
      };
    }

    case 'ADD_CHAT_MSG': {
      const list = [...state.list];
      if (Array.isArray(action.data)) {
        list.push([...action.data]);
      } else {
        list.push(action.data);
      }
      while (list.length > 50) list.shift();

      return {
        ...state,
        list,
      };
    }

    case 'UPDATE_CHAT_SHOWINPUT': {
      return {
        ...state,
        showInput: action.data,
      };
    }

    case 'UPDATE_CHAT_INPUT': {
      return {
        ...state,
        input: action.data,
      };
    }
  }
  return state;
};
