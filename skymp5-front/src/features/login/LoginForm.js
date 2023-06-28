import React, { useEffect, useState } from 'react';
import SkyrimButton from '../../components/SkyrimButton/SkyrimButton';
import SkyrimInput from '../../components/SkyrimInput';
import { toggleClass } from '../../utils/toggleClass';
import SkyrimHint from '../../components/SkyrimHint';

const LoginForm = props => {
  const [data, setData] = useState({
    email: localStorage.getItem('email') ? localStorage.getItem('email') : '',
    password: localStorage.getItem('password') ? localStorage.getItem('password') : ''
  });
  const [isButtonDisabled, setButtonDisabled] = useState(!(localStorage.getItem('email') && localStorage.getItem('password')));
  const [isRemember, setRemember] = useState(true);
  const [isRegisterHintOpened, setRegisterHintOpened] = useState(false);
  const [isRememberHintOpened, setRememberHintOpened] = useState(false);
  const [isPasswordShowed, setPasswordShowed] = useState(false);
  const handleInput = (e) => {
    setData({ ...data, [e.target.name]: e.target.value });
    if (data.email.length > 5 && data.password.length > 3) {
      setButtonDisabled(false);
    } else {
      setButtonDisabled(true);
    }
    if (e.target.value === '') {
      setButtonDisabled(true);
    }
  };
  const handleLogin = (credentials) => {
    if (isRemember) {
      localStorage.setItem('email', credentials.email);
      localStorage.setItem('password', credentials.password);
    }
  };
  useEffect(() => {
    const listener = (e) => {
      if (e.key === 'Enter' && !isButtonDisabled) {
        handleLogin(data);
      }
    };
    document.addEventListener('keypress', listener);
    return () => document.removeEventListener('keypress', listener);
  }, [data, isButtonDisabled]);
  return (
        <div className={'login-form--content_main'}>
            <div className={'login-form--content_main__email'}>
                <div className={'login-form--content_main__label'}>
                    <span className={'login-form--content_main__label___text'}>{props.locale.LOGIN.EMAIL}</span>
                    <img src={require('../../img/mail.svg').default} alt=""/>
                </div>
                <SkyrimInput defaultValue={localStorage.getItem('email')} onInput={handleInput} placeholder={props.locale.LOGIN.EMAIL_PLACEHOLDER} type={'text'} name={'email'}/>

            </div>
            <div className={'login-form--content_main__password'}>
                <div className={'login-form--content_main__label'}>
                    <span className={'login-form--content_main__label___text'} >{props.locale.LOGIN.PASSWORD}</span>
                    <img src={require('../../img/password.svg').default} alt=""/>
                </div>
                <SkyrimInput defaultValue={localStorage.getItem('password')} onInput={handleInput} placeholder={props.locale.LOGIN.PASSWORD_PLACEHOLDER} type={isPasswordShowed ? 'text' : 'password'} name={'password'}/>
            </div>
            <div className={'login-form--content_main__footer'}>
                <div className={'login-form--content_main__label login-form--content_main__container'}>
                    <SkyrimHint
                        text={props.locale.LOGIN.REMEMBER_HINT}
                        isOpened={isRememberHintOpened}
                        left={true}
                    />
                    <span className={'login-form--content_main__label___text'}>{props.locale.LOGIN.REMEMBER_PLACEHOLDER}</span>
                    <label
                            htmlFor="cbtest"
                            className={'checkbox active'}
                           onClick={(e) => {
                             if (isRemember) setRemember(false);
                             else setRemember(true);
                             toggleClass(e.target, 'active');
                           }}
                           onMouseOver={() => {
                             setRememberHintOpened(true);
                           }
                           }
                           onMouseOut={() => setRememberHintOpened(false)}
                    />

                </div>
                <div
                    className={`skymp-input button ${!isButtonDisabled ? 'disabled' : ''}`}
                    onClick={() => {
                      if (isButtonDisabled) {
                        props.setRegister(true);
                      }
                    }}
                        onMouseOver={() => {
                          if (isButtonDisabled) { setRegisterHintOpened(true); }
                        }
                        }
                        onMouseOut={() => setRegisterHintOpened(false)}
                >
                    <span className={'skymp-input_text'}>{props.locale.LOGIN.REGISTER_BUTTON}</span>
                        <SkyrimHint
                            text={props.locale.LOGIN.REGISTER_HINT}
                            isOpened={isRegisterHintOpened}
                            left={false}
                        />
                </div>
            </div>
            <div className={'login-form--content_main__button'}>
                <SkyrimButton disabled={isButtonDisabled} onClick={(e) => {
                  handleLogin(data);
                }} text={props.locale.LOGIN.LOGIN_BUTTON_TEXT}/>
            </div>
        </div>
  );
};

export default LoginForm;
