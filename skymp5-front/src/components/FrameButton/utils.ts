export const getFrameButtonClassName = (disabled: boolean): string => {
  return `skymp-button ${disabled ? 'disabled' : 'active'}`;
};

export const shouldHandleFrameButtonClick = (disabled: boolean): boolean => {
  return !disabled;
};
