import { Octokit } from '@octokit/rest';

export interface Pull {
  number: number;
  headLabel: string;
  authorLogin: string;
  title: string;
}

export async function getPullsWithLabel(repo: { owner: string, repo: string }, requestedLabel: string, octokit: Octokit) {
  const { data: pulls } = await octokit.pulls.list({
    ...repo,
    state: 'open',
  });
  const result: Pull[] = [];
  for (const pull of pulls) {
    if (!pull.labels.find((label) => label.name == requestedLabel)) {
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
