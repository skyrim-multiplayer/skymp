export const CONFIG_SCHEMA = {
  version: 1,
  sections: [
    {
      id: 'paths',
      title: 'Paths',
      description:
        'These settings tell the build assistant where SkyMP outputs live and where your local Skyrim installation is located.',
      fields: [
        {
          key: 'skyrimRoot',
          label: 'Skyrim Root',
          input: 'text',
          placeholder: 'C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition',
          description: 'Path to the Skyrim folder that contains SkyrimSE.exe.',
          required: false,
        },
        {
          key: 'buildDir',
          label: 'Build Directory',
          input: 'text',
          placeholder: 'C:/projects/skymp/build',
          description: 'Override the default SkyMP CMake build directory.',
          required: false,
        },
        {
          key: 'nirnLabOutputDir',
          label: 'NirnLab Output Directory',
          input: 'text',
          placeholder: 'D:/src/NirnLabUIPlatform/build/dist/Release',
          description: 'Optional override for the NirnLab UI runtime output directory.',
          required: false,
        },
        {
          key: 'skseLoaderPath',
          label: 'SKSE Loader Path',
          input: 'text',
          placeholder:
            'C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition/skse64_loader.exe',
          description:
            'Optional override for the SKSE launcher used by runtime supervision. Defaults to skse64_loader.exe inside Skyrim Root.',
          required: false,
        },
        {
          key: 'pluginsTxtPath',
          label: 'Plugins.txt Path',
          input: 'text',
          placeholder: 'C:/Users/you/AppData/Local/Skyrim Special Edition/Plugins.txt',
          description:
            'Optional override for the Bethesda-style plugin enable list used by the Mods tab. Defaults to %LOCALAPPDATA%/Skyrim Special Edition/Plugins.txt on Windows.',
          required: false,
        },
      ],
    },
  ],
};

export function getConfigSchema() {
  return CONFIG_SCHEMA;
}

export function getConfigFields() {
  return CONFIG_SCHEMA.sections.flatMap((section) =>
    section.fields.map((field) => ({
      ...field,
      sectionId: section.id,
      sectionTitle: section.title,
    })),
  );
}

export function getConfigFieldKeys() {
  return getConfigFields().map((field) => field.key);
}
