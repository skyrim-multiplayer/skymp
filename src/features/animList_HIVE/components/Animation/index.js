import React from 'react';

import { connect } from 'react-redux';

import './styles.sass';

const svg = (
  <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor" xmlns="http://www.w3.org/2000/svg">
    <path d="M16 6.18167L10.1863 5.79945L7.99681 0.29895L5.80734 5.79945L0 6.18167L4.45419 9.96373L2.99256 15.7009L7.99681 12.5377L13.0011 15.7009L11.5395 9.96373L16 6.18167Z" />
  </svg>  
);

const Animation = (props) => {

  // При нажатии на анимацию (или группу)
  const handleClick = () => {
    props.updateSearch('');

    if (props.data.type === 'group') {
      props.selectGroup(props.data.name);
    } else {
      initAnimation(props.data.name);
    }
  }

  // Инициализация анимации (демонстративно)
  const initAnimation = (name) => {
    alert(`Запустилась анимация: ${name}`);
  }

  // При добавлении/удалении анимации из "Избранное"
  const mark = (event) => {
    event.stopPropagation();

    let newAnimations = props.animations.items;

    // Добавление/удаление анимации из "Избранное"
    newAnimations.forEach((item, index) => {
      if (item.name === props.data.name) {
        newAnimations[index].marked = !item.marked;

        if (newAnimations[index].parents.includes('Избранное')) { 
          newAnimations[index].parents.splice(newAnimations[index].parents.indexOf('Избранное'), 1);
        } else {
          newAnimations[index].parents.push('Избранное');
        }
      }
    });

    props.updateAnimations(newAnimations);
  }

  // Получаем количество анимаций в группе
  const getCount = () => {
    return props.animations.items.filter(animation => animation.parents.includes(props.data.name)).length;
  }

  return (
    <div className={ `animation ${props.data.type === 'group' ? 'animation--group' : 'animation--item'}` } onClick={handleClick}>
      <span className="animation__text">{ props.data.name }</span>
      { props.data.type === 'group' ? 
        <span className="animation__count">{ getCount() }</span> : 
        <span className={ `animation__mark ${props.data.marked ? 'active' : ''} ` } onClick={mark}>{ svg }</span> }
    </div>
  );

}

const mapStateToProps = (state) => {
  const defaultState = state.animListHiveReducer;

  return {
    animations: defaultState.animations,
    selectedGroup: defaultState.selectedGroup,
    groupIsSelected: defaultState.groupIsSelected
  }
}

const mapDispatchToProps = (dispatch) => ({
  selectGroup: (data) =>
    dispatch({
      type: 'SELECT_GROUP',
      data,
    }),
  selectAnimation: (data) =>
    dispatch({
      type: 'SELECT_ANIMATION',
      data,
    }),
  updateAnimations: (data) =>
    dispatch({
      type: 'UPDATE_ANIMATIONS',
      data,
    }),
  updateSearch: (data) =>
    dispatch({
      type: 'UPDATE_SEARCH',
      data,
    }),
})

export default connect(mapStateToProps, mapDispatchToProps)(Animation);