const defaultState = {
  show: true,
  toggle: true,
  currentGroup: 0,
  groups: {
    titles: ['Все', 'RP', 'Non-RP', 'Гильдия', 'Команда'],
    lists: [
      [ // Все

      ],
      [ // RP

      ],
      [ // Non-RP

      ],
      [ // Гильдия

      ],
      [ // Команда
      ],
    ]
  },
  input: ''
};

export const chatHiveReducer = (state = defaultState, action) => {
  switch (action.type) {
    case "UPDATE_CHATHIVE_SHOW":
      return {
        ...state,
        show: action.data,
      }

    case 'UPDATE_CHATHIVE_TOGGLE':
      return {
        ...state,
        toggle: action.data,
      }

    case 'UPDATE_CHATHIVE_CURRENTGROUP':
      return {
        ...state,
        currentGroup: action.data,
      }
    
    case 'UPDATE_CHATHIVE_INPUT':
      return {
        ...state,
        input: action.data,
      }

    case 'ADD_CHATHIVE_MESSAGE':
      const limit = 50
      const lists = [...state.groups.lists]
      if(action.data.group === 0)
        action.data.group = 1
      if(Array.isArray(action.data.message)) {
        lists[action.data.group].push([...action.data.message])
        lists[0].push([...action.data.message])
      } else {
        lists[action.data.group].push(action.data.message)
        lists[0].push(action.data.message)
      }
      while (lists[action.data.group].length > limit) 
        lists[action.data.group].shift()

      while (lists[0].length > limit) 
        lists[0].shift()

      return {
        ...state,
        groups: {
          ...state.groups,
          lists,
        },
      };
  }
  return state;
}
