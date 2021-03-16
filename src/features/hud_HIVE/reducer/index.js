const defaultState = {
  show: true,
  stats: {
    show: true,
    hunger: 100,
    thirst: 100,
  }
}

export const hudHiveReducer = (state = defaultState, action) => {
  switch (action.type) {
    case 'UPDATE_HUD_SHOW': {
      return {
        ...state,
        show: action.data
      }
    }

    case 'UPDATE_HUD_STATS': {
      const { hunger, thirst } = action.data

      return {
        ...state,
        stats: {
          ...state.stats,
          hunger,
          thirst,
        }
      }
    }
  }

  return state
}
