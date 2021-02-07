import React from 'react';

import { connect } from 'react-redux';

import './styles.sass';

const SpawnItem = (props) => {

  const onSpawnSelect = () => {
    props.updateShow(false);

    window.mp.send('cef::spawnHIVE:send', {
      title: props.data.title,
      locationID: props.data.locationID
    })
  }

  return (
    <div className="spawn__item" onClick={onSpawnSelect}>
      <div className="item__image">
        <img src={ props.data.image } alt=""/>
      </div>
      <h3 className="item__title">{ props.data.title }</h3>
      <p className="item__description">{ props.data.description }</p>
    </div>
  );

}

const mapStateToProps = (state) => ({})

const mapDispatchToProps = (dispatch) => ({
  updateShow: (data) =>
    dispatch({
      type: 'UPDATE_HIVESPAWN_SHOW',
      data,
    })
})

export default connect(mapStateToProps, mapDispatchToProps)(SpawnItem);