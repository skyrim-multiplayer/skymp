export interface SendMessageEvent<Message> {
    message: Message;
    reliability: 'unreliable' | 'reliable';
}
