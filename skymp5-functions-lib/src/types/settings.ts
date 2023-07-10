export interface ChatSettings {
  hearingRadiusNormal?: number;
  whisperDistance?: number;
  shoutDistance?: number;
  minDistanceToChange?: number;
};

export interface ServerSettings {
  sweetpieChatSettings: ChatSettings
}
