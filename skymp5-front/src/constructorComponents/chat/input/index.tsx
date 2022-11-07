import React from 'react';
import './styles.scss';

interface ChatInputProps {
  onChange: (value: string) => void,
  onFocus: () => void,
  onBlur: () => void,
  placeholder: string,
  fontSize: number,
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
        style={{
          fontSize: props.fontSize
        }}
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
