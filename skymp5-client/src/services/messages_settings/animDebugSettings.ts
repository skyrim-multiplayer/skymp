export interface AnimTextOutput {
  isActive?: boolean;
  itemCount?: number;
  startPos?: { x: number, y: number },
  yPosDelta?: number,
}

export interface AnimDebugSettings {
  isActive?: boolean;
  textOutput?: AnimTextOutput,
  animKeys?: { [index: number]: string };
}
