import { ComponentProps, FormEvent } from 'react';
import { DefaultUIElementProps } from '.';

export interface SkyrimHintProps {
    isOpened: boolean;
    active: string;
    text: string;
    left: boolean;
}

export interface DefaultButtonComponentProps extends React.ComponentProps<'button'>, DefaultUIElementProps {
    name: string;
}

export interface ImageButtonProps extends DefaultButtonComponentProps {
    src: string
}

export interface SkyrimButtonProps extends DefaultButtonComponentProps {
    text: string
}
export interface SkyrimSliderProps extends DefaultButtonComponentProps {
    setValue: (value: number) => void,
    sliderValue: number,
    marks: number[],
    min: number,
    max: number,
    text: string
}

export interface FrameButtonProps extends DefaultButtonComponentProps {
    variant: string;
    text: string;
}
