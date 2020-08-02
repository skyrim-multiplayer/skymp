export interface SendTarget {
  send(msg: Record<string, unknown>, reliable: boolean): void;
}
