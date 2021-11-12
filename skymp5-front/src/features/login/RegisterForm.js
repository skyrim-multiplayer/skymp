import React, { useEffect, useState } from 'react';
import SkyrimButton from '../../components/SkyrimButton/SkyrimButton';
import SkyrimInput from '../../components/SkyrimInput';
import { toggleClass } from '../../utils/toggleClass';
import SkyrimHint from '../../components/SkyrimHint';

const RegisterForm = props => {
  const [data, setData] = useState({
    email: '',
    password: ''
  });
  const [isButtonBack, setButtonBack] = useState(true);
  const [isRegisterHintOpened, setRegisterHintOpened] = useState(false);
  const [isPasswordHintOpened, setPasswordHintOpened] = useState(false);
  const [isButtonDisabled, setButtonDisabled] = useState(true);
  const handleInput = (e) => {
    if (e.target.name === 'password_verify') {
      console.log(data.password, e.target.value);
      setButtonDisabled(data.password !== e.target.value);
    } else {
      setData({ ...data, [e.target.name]: e.target.value });
      if (data.email.length > 5 && data.password.length > 3) {
        console.log(true);
        setButtonBack(false);
      } else {
        setButtonBack(true);
      }
    }

    console.log(data);
  };
  const handleSubmit = () => {
    console.log('submit', data);
  };
  useEffect(() => {
    const listener = (e) => {
      console.log(e.key, isButtonBack, e.key === 'Enter');
      if (e.key === 'Enter' && !isButtonBack) {
        handleSubmit();
      }
    };
    document.addEventListener('keypress', listener);
    return () => document.removeEventListener('keypress', listener);
  }, [data, isButtonBack]);
  return (
        <div className={'login-form--content_main'}>
            <div className={'login-form--content_main__email'}>
                <div className={'login-form--content_main__label'}>
                    <span className={'login-form--content_main__label___text'}>{props.locale.LOGIN.EMAIL}</span>
                    <img src={require('../../img/mail.svg').default} alt=""/>
                </div>
                <SkyrimInput onInput={handleInput} placeholder={props.locale.LOGIN.EMAIL_PLACEHOLDER} type={'text'} name={'email'}/>

            </div>
            <div className={'login-form--content_main__password'}>
                <div className={'login-form--content_main__label'}>
                    <span className={'login-form--content_main__label___text'} >{props.locale.LOGIN.PASSWORD}</span>
                    <img src={require('../../img/password.svg').default} alt=""/>
                </div>
                <SkyrimInput onInput={handleInput} placeholder={props.locale.LOGIN.PASSWORD_PLACEHOLDER} type={'password'} name={'password'}/>
            </div>
            <div className={'login-form--content_main__password'}>
                <div className={'login-form--content_main__label'}>
                    <span className={'login-form--content_main__label___text'} >{props.locale.LOGIN.PASSWORD_VERIFY}</span>
                    <img src={require('../../img/password.svg').default} alt=""/>
                </div>
                <SkyrimInput onInput={handleInput} placeholder={props.locale.LOGIN.PASSWORD_VERIFY_PLACEHOLDER} type={'password'} name={'password_verify'}/>
            </div>
            <div className={'login-form--content_main__button'}>
                <SkyrimButton disabled={!isButtonBack && isButtonDisabled} onClick={(e) => {
                  if (isButtonBack) {
                    props.setRegister(false);
                  } else {
                    console.log(data);
                  }
                }} text={isButtonBack ? props.locale.LOGIN.BACK : props.locale.LOGIN.LOGIN_BUTTON_TEXT}/>
            </div>
        </div>
  );
};

export default RegisterForm;
