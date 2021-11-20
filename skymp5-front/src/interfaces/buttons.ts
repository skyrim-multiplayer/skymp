export interface SkyrimHintProps {
    isOpened: boolean;
    active: string;
    text: string;
    left: boolean;
}

export interface SkyrimInputProps extends DefaultButtonComponentProps {
    labelText: string;
    type: React.HTMLInputTypeAttribute;
    initialValue: string | number | readonly string[];
    placeholder: string;
    onInput: (value?: string | boolean) => void;
}

export interface DefaultButtonComponentProps {
    name: string;
    width?: number;
    height?: number;
    disabled?: boolean;
    onClick?: (value?: string | boolean) => void;
}

export interface ImageButtonProps extends DefaultButtonComponentProps{
    src: string
}

export interface SkyrimButtonProps extends DefaultButtonComponentProps {
    text: string
}

export interface FrameButtonProps extends DefaultButtonComponentProps {
    variant: string;
    text: string;
}
