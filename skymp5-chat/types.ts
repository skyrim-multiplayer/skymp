export interface Mp {
  makeProperty(name: string, config: MpPropertyConfig): void
  set(formId: number, propertyName: string, value: unknown): void
  getActorPos(actorId: number): [number, number, number] | null
}

export interface MpPropertyConfig {
  isVisibleByOwner: boolean
  isVisibleByNeighbors: boolean
  updateOwner: string
  updateNeighbor: string
}

export interface PlayerState {
  id: number
  actorId: number
  name: string
  factions: string[]
}

export interface Store {
  get(id: number): PlayerState | null
  getAll(): PlayerState[]
}
