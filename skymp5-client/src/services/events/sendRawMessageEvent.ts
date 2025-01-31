export interface SendRawMessageEvent {
    rawMessage: ArrayBuffer;
    reliability: 'unreliable' | 'reliable';
}
