import React, { useEffect } from 'react';

import { connect } from 'react-redux';

import Animation from './components/Animation';

import './styles.sass';

const initKeyCodes = [115, 1099]; // s - на англ., ы - на рус. 

const AnimListHIVE = (props) => {

  const [windowX, setWindowX] = React.useState(window.innerWidth - 350 - 40)
  const [windowY, setWindowY] = React.useState(window.innerHeight / 2 - 425 / 2)
  const windowBlock = React.useRef(null)

  useEffect(() => {
    const onKeypress = (event) => {
      // Если ввод был внутри input
      let isInput = String(event.target).includes('Input');

      if (initKeyCodes.includes(event.keyCode) && !isInput) {
        props.toggleWindow();
      }
    }

    document.addEventListener('keypress', onKeypress);
  
    return () => {
      document.removeEventListener('keypress', onKeypress);
    }
  });

  // При вводе
  const handleChange = (event) => {
    let searchValue = event.target.value.toLowerCase();

    props.updateSearch(searchValue);
  }

  // Получаем отфильтрованные анимации
  const getAnimations = () => {
    let items;

    if (props.search && !props.groupIsSelected) {
      items = props.animations.items.filter(animation => animation.name.toLowerCase().includes(props.search));
    } else {
      if (props.groupIsSelected) {
        items = props.animations.items.filter(animation => animation.parents.includes(props.selectedGroup));
        items = items.filter(animation => animation.name.toLowerCase().includes(props.search));
      } else {
        items = props.animations.groups;
      }
    }

    return items;
  }

  const AnimationComponents = getAnimations().map((animation, index) => {
    return <Animation key={index} data={animation} />;
  });
  
  return (
    props.windowIsOpen &&
    <div
      id="animations"
      ref={windowBlock}
      style={{
        left: windowX,
        top: windowY,
      }}
    >
      <div className="animations__inner">
        <div className="animations__header">
          <h1
            className="title"
            onMouseDown={(e) => {
              window.isMoveWindow = true
              window.moveWindowTranslateX = e.clientX - windowBlock.current.offsetLeft
              window.moveWindowTranslateY = e.clientY - windowBlock.current.offsetTop
              window.moveWindowWidth = windowBlock.current.clientWidth
              window.moveWindowHeight = windowBlock.current.clientHeight
              window.moveWindow = (clientX, clientY) => {
                let left = clientX - window.moveWindowTranslateX
                let top = clientY - window.moveWindowTranslateY
                if(left + window.moveWindowWidth > window.innerWidth)
                  left = window.innerWidth - window.moveWindowWidth
                if(top + window.moveWindowHeight > window.innerHeight)
                  top = window.innerHeight - window.moveWindowHeight
                setWindowX(left)
                setWindowY(top)
              }
            }}
          >{ props.groupIsSelected ? props.selectedGroup : 'Анимации' }</h1>
          <button className="close" onClick={props.toggleWindow}>&times;</button>
          <input type="text" value={props.search} placeholder="Поиск анимаций" onChange={handleChange} />
        </div>
        <div className="animations__list">
          { AnimationComponents.length ?
            AnimationComponents :
            <p>Список анимаций пуст</p>
          }
        </div>
        { (props.groupIsSelected) &&
          <span className="back" onClick={props.goBack}>Назад</span>
        }
      </div>
    </div>
  );

}

const mapStateToProps = (state) => {
  const defaultState = state.animListHiveReducer;

  return {
    animations: defaultState.animations,
    selectedGroup: defaultState.selectedGroup,
    groupIsSelected: defaultState.groupIsSelected,
    windowIsOpen: defaultState.windowIsOpen,
    search: defaultState.search
  }
}

const mapDispatchToProps = (dispatch) => ({
  selectGroup: (data) =>
    dispatch({
      type: 'SELECT_GROUP',
      data
    }),
  toggleWindow: (data) =>
    dispatch({
      type: 'TOGGLE_WINDOW',
      data
    }),
  goBack: (data) =>
    dispatch({
      type: 'GO_BACK',
      data
    }),
  updateSearch: (data) =>
    dispatch({
      type: 'UPDATE_SEARCH',
      data
    })
})

export default connect(mapStateToProps, mapDispatchToProps)(AnimListHIVE);