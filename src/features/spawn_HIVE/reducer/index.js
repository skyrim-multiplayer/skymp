const defaultState = {
  show: false
};

export const spawnHiveReducer = (state = defaultState, action) => {

  switch (action.type) {
    case 'UPDATE_HIVESPAWN_SHOW': {
      return {
        ...state,
        show: action.data
      }
    }
  }

  return state;

}
