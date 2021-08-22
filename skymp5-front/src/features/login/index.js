import React from "react";

import './styles.scss'
import Frame from "../../components/SkyrimFrame";

const LoginPage = props => {
    return (
        <div className={'login'} >
            <div className={'login-form'}>
                <Frame />
                <div className={'login-form--content'}>
                    <div className={'login-form--content_header'}>

                    </div>
                </div>
            </div>
        </div>
    )
}

export default LoginPage