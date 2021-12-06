import { Octokit } from '@octokit/rest';
import * as pull_manager from './pull_manager';
import * as pull_merger from './pull_merger';

const octokit = new Octokit();


(async function() {

// const { data: pr } = await octokit.rest.pulls.get({
//   owner: 'skyrim-multiplayer',
//   repo: 'skymp',
//   pull_number: 123,
// });

// // console.log(pullRequest);
// console.log(pr.number, pr.labels);

// const { data: prList } = await octokit.pulls.list({
//   owner: 'skyrim-multiplayer',
//   repo: 'skymp',
//   state: 'open',
// });

// for (const pr of prList) {
//   pr.labels
// }

await pull_merger.mergePull({headLabel: 'kek', number: 123,});

})().catch((err) => console.error(err));
