import React, { useEffect } from 'react';
import './styles.scss';

interface ChatInputProps {
  onChange: (value: string) => void,
  onFocus: () => void,
  onBlur: () => void,
  placeholder: string,
}

const ChatInput = React.forwardRef<HTMLSpanElement, ChatInputProps>(
  function ChatInput (props, ref) {
    const handleInput = (event: React.FormEvent<HTMLDivElement>) => {
      const target = event.target as HTMLDivElement;
      // Fix placeholder when new line was used
      if (target.innerHTML === '<br>') {
        target.innerHTML = '';
      }
      props.onChange(target.innerText);
      // if (
      // // @ts-ignore (Didn't find a type, where inputType is available)
      //   event.nativeEvent.inputType !== 'insertLineBreak' &&
      // // @ts-ignore
      // event.nativeEvent.inputType !== 'deleteContentBackward'
      // ) {  };
    };
    return (
    <div className='chat-input--wrapper'>
      <span
        className={'chat-input--text show'}
        contentEditable={true}
        placeholder={'Ваше сообщение...'}
        onInput={handleInput}
        ref={ref}
        id={'chatInput'}
        onPaste={(event) => {
          // Paste only text
          event.preventDefault();
          document.execCommand(
            'insertText',
            false,
            event.clipboardData.getData('text/plain')
          );
        }}
        onFocus={props.onFocus}
        onBlur={props.onBlur}
      />
    </div>
    );
  }
);
export default ChatInput;
