export interface SendTarget {
    send(msg: any, reliable: boolean);
}