import React from 'react';

import './SkyrimSlider.scss';
import { SkyrimSliderProps } from '../../interfaces';
import ReactSlider from 'react-slider';

export const SkyrimSlider = ({
  setValue,
  text,
  sliderValue,
  min,
  max,
  marks
}: SkyrimSliderProps) => {
  return (
    <div className="skyrimSlider">
      <span className="skyrimSlider_text">{text}</span>
      <ReactSlider
        className="skyrimSlider_slider"
        thumbClassName="skyrimSlider_thumb"
        min={min}
        max={max}
        value={sliderValue}
        marks={marks}
        onChange={(value) => setValue(value)}
        renderTrack={(props, state) => {
          return (
            <div {...props} className={'skyrimSlider_track'}>
              <div className={'skyrimSlider_track_inner'} />
            </div>
          );
        }}
        renderMark={(props) => {
          if ((props.key as number) % 2 === 0) {
            return (
              <div {...props} className={'skyrimSlider_mark even'}>
                <span className={'skyrimSlider_mark__number'}>{props.key}</span>
              </div>
            );
          } else {
            return <span {...props} className={'skyrimSlider_mark odd'} />;
          }
        }}
      />
    </div>
  );
};
