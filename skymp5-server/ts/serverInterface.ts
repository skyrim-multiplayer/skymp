export interface ServerInterface {
  sendCustomPacket(userId: number, type: string, content: Record<string, unknown>): void

  setRaceMenuOpen(formId: number, open: boolean): void
  createActor(formId: number, pos: number[], angleZ: number, cellOrWorld: number, userProfileId?: number): number

  destroyActor(formId: number): void
  setUserActor(userId: number, actorFormId: number): void
  getUserActor(userId: number): number
  getActorsByProfileId(profileId: number): number[]
  setEnabled(formId: number, enabled: boolean): void
}
