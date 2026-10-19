declare module '*.png' {
  const value: string;
  export = value;
}

declare module '*.svg' {
  const value: string;
  export = value;
}

declare module '*.wav' {
  const src: string;
  export default src;
}

declare module '*.scss' {
  const value: Record<string, string>;
  export default value;
}
