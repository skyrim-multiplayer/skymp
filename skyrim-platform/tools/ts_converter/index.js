'use strict';
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '../../..');
const functionsDumpPath = path.join(root, 'skyrim-platform/src/platform_se/codegen/convert-files/FunctionsDump.txt');
const definitionsPath = path.join(root, 'skyrim-platform/src/platform_se/codegen/convert-files/Definitions.txt');
const outputPath = path.join(root, 'skyrim-platform/src/platform_se/codegen/convert-files/skyrimPlatform.ts');

const j = JSON.parse(fs.readFileSync(functionsDumpPath, 'utf8'));
const definitions = fs.readFileSync(definitionsPath, 'utf8');

const ignored = new Set(['TESModPlatform.Add', 'Math', 'MpClientPlugin']);
const functionNameOverrides = { getplayer: 'getPlayer' };
const dumped = new Set();

// Mirrors TSConverter.cpp: uppercase/lowercase first char, then if rest is
// all-same-case force it lowercase, otherwise keep as-is.
function prettify(name, transformFirst = c => c.toUpperCase()) {
  const firstChar = transformFirst(name[0]);
  const rest = name.slice(1);
  const needsLower = rest === rest.toLowerCase() || rest === rest.toUpperCase();
  return firstChar + (needsLower ? rest.toLowerCase() : rest);
}

function parseReturnValue(rawType, objectTypeName) {
  if (rawType === 'Int' || rawType === 'Float') return 'number';
  if (rawType === 'Bool') return 'boolean';
  if (rawType === 'String') return 'string';
  if (rawType === 'IntArray' || rawType === 'FloatArray') return 'number[] | null';
  if (rawType === 'BoolArray') return 'boolean[] | null';
  if (rawType === 'StringArray') return 'string[] | null';
  if (rawType === 'None') return 'void';
  if (rawType === 'Object') return (objectTypeName ? prettify(objectTypeName) : 'Form') + ' | null';
  if (rawType === 'ObjectArray') return 'PapyrusObject[] | null';
  return '';
}

function dumpFunction(className, f, isGlobal) {
  if (ignored.has(`${className}.${f.name}`)) return '';

  let funcName = functionNameOverrides[f.name] || f.name;
  const prettyName = prettify(funcName, c => c.toLowerCase());
  // TSConverter lowercases funcName after outputting the name but before the
  // argument loop — this affects the isSetMotionType and isAddOrRemove checks.
  funcName = funcName.toLowerCase();

  const isAddOrRemove = funcName === 'additem' || funcName === 'removeitem';

  const args = f.arguments.map((arg, i) => {
    const argType = (funcName === 'setmotiontype' && i === 0)
      ? 'MotionType'
      : parseReturnValue(arg.type.rawType, arg.type.objectTypeName || '');
    const argName = arg.name === 'in' ? '_in' : arg.name;
    return `${argName}: ${argType}`;
  });

  let returnType = parseReturnValue(f.returnType.rawType, f.returnType.objectTypeName || '');
  if (f.isLatent && !isAddOrRemove) returnType = `Promise<${returnType}>`;

  return `  ${isGlobal ? 'static ' : ''}${prettyName}(${args.join(', ')}): ${returnType}\n`;
}

function dumpType(data) {
  if (ignored.has(data.name) || dumped.has(data.name)) return '';

  let out = '';
  if (data.parent !== undefined) {
    const parentData = j.types[data.parent];
    if (parentData) out += dumpType({ ...parentData, name: data.parent });
  }

  const prettyName = prettify(data.name);
  const prettyParent = data.parent !== undefined ? prettify(data.parent) : 'PapyrusObject';

  out += `\n// Based on ${prettyName}.pex\n`;
  out += `export declare class ${prettyName} extends ${prettyParent} {\n`;
  out += `  static from(papyrusObject: PapyrusObject | null): ${prettyName} | null\n`;
  for (const f of data.memberFunctions) out += dumpFunction(data.name, f, false);
  for (const f of data.globalFunctions) out += dumpFunction(data.name, f, true);
  out += '}\n';

  dumped.add(data.name);
  return out;
}

if (!j.types['WorldSpace']) {
  j.types['WorldSpace'] = { parent: 'Form', globalFunctions: [], memberFunctions: [] };
}

let output = '\n';
output += '/* eslint-disable @typescript-eslint/adjacent-overload-signatures */\n';
output += '/* eslint-disable @typescript-eslint/no-namespace */\n';
output += '// Generated automatically. Do not edit.\n';
output += '\n';
output += definitions;

// Use sorted keys to match nlohmann/json std::map iteration order in TSConverter.cpp
for (const typeName of Object.keys(j.types).sort()) {
  output += dumpType({ ...j.types[typeName], name: typeName });
}

fs.writeFileSync(outputPath, output, 'utf8');
