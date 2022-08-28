import React from 'react';
import { SkyrimFrame } from '../../../components/SkyrimFrame/SkyrimFrame';
import { SkyrimSlider } from '../../../components/SkyrimSlider/SkyrimSlider';
import CheckBox from '../../checkbox/index';
import './styles.scss';

const Settings = (props: {
  fontSize: number,
  setFontSize: (size: number) => void,
  isSoundsDisabled: boolean,
  setDisableSounds: (disable: boolean) => void,
  showSendButton: boolean,
  setShowSendButton: (value: boolean) => void,
}) => {
  return (
    <div className='chat-settings'>
      <div className='content'>
        <SkyrimSlider text={'размер шрифта'} name={'fontSize'} min={14} max={22} setValue={(value) => props.setFontSize(value)} sliderValue={props.fontSize} marks={[14, 15, 16, 17, 18, 19, 20, 21, 22]}/>
        <CheckBox text={'звуки дайсов'} initialValue={!props.isSoundsDisabled} setChecked={(value) => props.setDisableSounds(!value)} disabled={false} />
        <CheckBox text={'кнопка отправки'} initialValue={props.showSendButton} setChecked={props.setShowSendButton} disabled={false} />
      </div>
      <SkyrimFrame width={512} height={360} header={false} name={'Settings'}/>
    </div>
  );
};

export default Settings;
