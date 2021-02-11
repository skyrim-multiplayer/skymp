import data from '../data.js'

const defaultState = {
  animations: data,
  selectedGroup: '',
  groupIsSelected: false,
  windowIsOpen: true,
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
