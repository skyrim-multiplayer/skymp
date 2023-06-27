import React from 'react';

import { DefaultUIElementProps } from '.';

export interface SkyrimInputProps
  extends React.ComponentProps<'input'>,
    DefaultUIElementProps {
  labelText: string;
  type: React.HTMLInputTypeAttribute;
  initialValue: string | number | readonly string[];
  placeholder: string;
  name: string;
  width?: number;
  height?: number;
}
