export interface SkyrimHintProps {
    isOpened: boolean;
    active: string;
    text: string;
    left: boolean;
}

export interface DefaultButtonComponentProps {
    name: string;
    width?: number;
    height?: number;
    disabled?: boolean;
    onClick?: (any?: any) => void;
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

export interface ButtonItemProps extends DefaultButtonComponentProps {
    name: string;
    text?: string;
    variant?: string;
    url?: string;
}
