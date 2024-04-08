export interface Mod {
    filename: string;
    size: number;
    crc32: number;
};

export interface ServerManifest {
    versionMajor: number;
    mods: Mod[];
    loadOrder: string[];
};
