import { Form } from './form';

export class Cell extends Form {
  constructor(formId: number) {}
  GetLocation(): number | undefined;
  GetFlags(): undefined | CellFlags[];
  IsInterior(): boolean;
}

export enum CellFlags {
  Interior = 0x0001,
  HasWater = 0x0002,
  CantTravelFromHere = 0x0004,
  NoLODWater = 0x0008,
  PublicArea = 0x0020,
  HandChanged = 0x0040,
  ShowSky = 0x0080,
  UseSkyLighting = 0x0100,
}
