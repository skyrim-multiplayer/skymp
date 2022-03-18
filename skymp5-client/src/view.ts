export interface View<T> {
  update(model: T): void;
  destroy(): void;
}
