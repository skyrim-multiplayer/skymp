import { WorldModel } from './model';

export interface ModelSource {
  getWorldModel(): WorldModel;
}
