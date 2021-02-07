const defaultState = {
  animations: {
    groups: [
      {
        name: 'Избранное',
        type: 'group'
      },
      {
        name: 'Действия',
        type: 'group'
      },
      {
        name: 'Позы',
        type: 'group'
      },
      {
        name: 'Стили походок',
        type: 'group'
      },
      {
        name: 'Муз. инструменты',
        type: 'group'
      },
      {
        name: 'Танцы',
        type: 'group'
      },
      {
        name: 'Остальное',
        type: 'group'
      }
    ],
    items: [
      {
        name: 'Анимация 1',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 2',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 3',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 4',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 5',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 6',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 7',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 8',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 9',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
      {
        name: 'Анимация 10',
        parents: ['Действия'],
        marked: false,
        type: 'item'
      },
    ]
  },
  selectedGroup: '',
  groupIsSelected: false,
  windowIsOpen: false,
  search: ''
}

export const animListHiveReducer = (state = defaultState, action) => {

  switch (action.type) {
    case 'TOGGLE_WINDOW': {
      return {
        ...state,
        windowIsOpen: !state.windowIsOpen
      }
    }
    case 'SELECT_GROUP': {
      return {
        ...state,
        selectedGroup: action.data,
        groupIsSelected: true
      }
    }
    case 'UPDATE_ANIMATIONS': {
      return {
        ...state,
        animations: {
          ...state.animations,
          items: action.data
        }
      }
    }
    case 'GO_BACK': {
      return {
        ...state,
        selectedGroup: defaultState.groupIsSelected,
        groupIsSelected: false
      }
    }
    case 'UPDATE_SEARCH': {
      return {
        ...state,
        search: action.data
      }
    }
  }

  return state;
  
}
