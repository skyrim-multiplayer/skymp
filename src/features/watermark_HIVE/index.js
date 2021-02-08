import React from 'react';

import { connect } from 'react-redux';

import './styles.sass';

const userSVG = (
  <svg viewBox="0 0 14 14" fill="currentColor" xmlns="http://www.w3.org/2000/svg">
    <g clip-path="url(#clip0)">
      <path d="M7.0009 0C5.00089 0 3.36597 1.63491 3.36597 3.63493C3.36597 5.63494 5.00089 7.26985 7.0009 7.26985C9.00091 7.26985 10.6358 5.63494 10.6358 3.63493C10.6358 1.63491 9.00091 0 7.0009 0Z" />
      <path d="M13.239 10.1745C13.1438 9.93641 13.0168 9.7142 12.8739 9.50784C12.1438 8.42847 11.0168 7.71419 9.74698 7.53958C9.58826 7.52372 9.41366 7.55544 9.28666 7.65069C8.61999 8.14275 7.82635 8.39672 7.00094 8.39672C6.17552 8.39672 5.38188 8.14275 4.71521 7.65069C4.58821 7.55544 4.41361 7.50783 4.25489 7.53958C2.98505 7.71419 1.8422 8.42847 1.12792 9.50784C0.985067 9.7142 0.858071 9.9523 0.762854 10.1745C0.715245 10.2698 0.731105 10.3809 0.778713 10.4761C0.90571 10.6983 1.06442 10.9206 1.20728 11.111C1.42949 11.4126 1.6676 11.6825 1.93745 11.9364C2.15966 12.1586 2.41363 12.365 2.66762 12.5713C3.92157 13.5079 5.42952 13.9999 6.98508 13.9999C8.54063 13.9999 10.0486 13.5078 11.3025 12.5713C11.5565 12.3809 11.8105 12.1586 12.0327 11.9364C12.2867 11.6825 12.5406 11.4126 12.7629 11.111C12.9216 10.9047 13.0645 10.6983 13.1914 10.4761C13.2708 10.3809 13.2866 10.2697 13.239 10.1745Z" />
    </g>
    <defs>
      <clipPath id="clip0">
        <rect width="14" height="14" fill="white"/>
      </clipPath>
    </defs>
  </svg>
);

const WatermarkHIVE = (props) => {
  return (
    <div id="watermark">
      <span className="title">HIVE RP</span>
      <div className="info">
        <span>ID: { props.userID }</span>
        <span>{ userSVG } { props.playersOnline }</span>
      </div>
    </div>
  );
}

const mapStateToProps = (state) => {
  const defaultState = state.watermarkHiveReducer;

  return {
    show: defaultState.show,
    userID: defaultState.userID,
    playersOnline: defaultState.playersOnline
  }
}

const mapDispatchToProps = (dispatch) => ({})

export default connect(mapStateToProps, mapDispatchToProps)(WatermarkHIVE);