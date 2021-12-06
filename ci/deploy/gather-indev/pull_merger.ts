// import { execa } from 'execa';
// const execa = require('execa').execa;
import { promisify } from 'util';
import { exec } from 'child_process';
const execAsync = promisify(exec);
import { Pull } from './pull_manager';
// import * from 'node';

// async function run(...args: string[]) {
async function run(args: string{
  // const sub = execa(args[0], args.slice(1));
  // sub.stdout.pipe(process.stdout);
  // sub.stderr.pipe(process.stderr);
  // return sub;
  const sub = execAsync(args);
}

export async function mergePull(pull: Pull) {
  // execa('git', ['pull', '--squash', `pull/${pull.number}/head`])
  await run('false');
}
