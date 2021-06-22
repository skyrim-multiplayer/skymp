export interface Tint {
  texturePath: string;
  argb: number;
  type: number;
}

export interface Look {
  isFemale: boolean;
  raceId: number;
  weight: number;
  skinColor: number;
  hairColor: number;
  headpartIds: number[];
  headTextureSetId: number;
  options: number[];
  presets: number[];
  tints: Tint[];
  name: string;
}
