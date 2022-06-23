import { Octokit } from '@octokit/rest';

const REPO = {
  owner: 'skyrim-multiplayer',
  repo: 'skymp',
};

export interface Pull {
  number: number;
  headLabel: string;
  authorLogin: string;
  title: string;
}

export async function getPullsWithLabel(octokit: Octokit) {
  const { data: pulls } = await octokit.pulls.list({
    ...REPO,
    state: 'open',
  });
  const result: Pull[] = [];
  for (const pull of pulls) {
    if (!pull.labels.find((label) => label.name == 'merge-to:' + process.env.DEPLOY_BRANCH)) {
      continue;
    }
    result.push({
      number: pull.number,
      headLabel: pull.head.label,
      authorLogin: pull.head.user.login,
      title: pull.title,
    });
  }
  return result;
}
