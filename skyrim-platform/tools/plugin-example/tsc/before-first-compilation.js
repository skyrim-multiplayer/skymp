let fs = require('fs');
let path = require('path');

let config = require('./config');
let tsconfig = JSON.parse(fs.readFileSync('./tsconfig-default.json'));
tsconfig.compilerOptions.baseUrl = path.join(config.seRoot, 'Data', 'Platform', 'Modules');
fs.writeFileSync('./tsconfig.json', JSON.stringify(tsconfig, null, 2));
