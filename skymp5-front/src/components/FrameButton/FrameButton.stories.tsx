import React from 'react';
import { ComponentStory, ComponentMeta } from '@storybook/react';
import { FrameButton } from './FrameButton';

export default {
  title: 'buttons/FrameButton',
  component: FrameButton,
} as ComponentMeta<typeof FrameButton>;

const Template: ComponentStory<typeof FrameButton> = (args) => <FrameButton {...args} />;

export const Default = Template.bind({});
Default.args = {
  disabled: false,
  variant: 'DEFAULT',
  text: 'Test',
  width: 320,
  height: 60
};

export const Left = Template.bind({});
Left.args = {
  disabled: false,
  variant: 'LEFT',
  text: 'Test',
  width: 320,
  height: 60
};

export const Right = Template.bind({});
Right.args = {
  disabled: false,
  variant: 'RIGHT',
  text: 'Test',
  width: 320,
  height: 60
};
