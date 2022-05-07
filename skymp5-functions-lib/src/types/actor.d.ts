import { ObjectReference } from './objectReference';
import { ActorValue } from 'skyrimPlatform';
import { BaseActorValues, Equipment, LocationalData } from './mp';

export class Actor extends ObjectReference {
  constructor(formId: number) {}
  DamageActorValue(av: ActorValue, value: number);
  GetBaseActorValues(): BaseActorValues;
  GetBounds(): ObjectBounds;
  GetEquipment(): Equipment;
  GetMaximumActorValues(): BaseActorValues;
  GetRaceId(): number;
  IsDead(): boolean;
  IsRaceMenuOpen(): boolean;
  IsRespawning(): boolean;
  IsWeaponDrawn(): boolean;
  Kill(killer?: Actor, shouldTeleport: boolean);
  RestoreActorValue(av: ActorValue, value: number);
  SetActorValuesPercentages(health: number, magicka: number, stamina: number, aggressor?: Actor);
  SetRaceMenuOpen(isOpen: boolean);
  SetSpawnPoint(locationalData: LocationalData);
  GetSpawnPoint(): LocationalData;
  GetRespawnTime(): number;
  SetRespawnTime(time: number);
}

export interface Appearance {
  isFemale: boolean;
  raceId: number;
  weight: number;
  skinColor: number;
  hairColor: number;
  headpartIds: number[];
  headTextureSetId: number;
  faceMorphs: number[];
  facePresets: number[];
  tints: Tint[];
  name: string;
}

export interface Tint {
  argb: number;
  texturePath: string;
  type: number;
}

export interface BaseActorValues {
  health: number;
  magicka: number;
  stamina: number;
  healRate: number;
  magickaRate: number;
  staminaRate: number;
  healRateMult: number;
  magickaRateMult: number;
  staminaRateMult: number;
}

export interface ObjectBounds {
  pos1: number[3];
  pos2: number[3];
}
