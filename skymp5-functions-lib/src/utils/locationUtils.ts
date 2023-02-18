export const sqr = (x: number) => x * x;

export type Vector3 = [number, number, number];
export type Position = Vector3;

export const squareDist = (pos1: Position, pos2: Position) =>
  sqr(pos1[0] - pos2[0]) + sqr(pos1[1] - pos2[1]) + sqr(pos1[2] - pos2[2]);
