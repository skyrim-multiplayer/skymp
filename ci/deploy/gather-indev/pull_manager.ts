import { Octokit } from '@octokit/rest';
import { Endpoints } from '@octokit/types/dist-types/generated/Endpoints';
// Endpoints["GET /repos/{owner}/{repo}/pulls"]["response"]

const REPO = {
  owner: 'skyrim-multiplayer',
  repo: 'skymp',
};

export interface Pull {
  number: number;
  headLabel: string;
}

export async function getPullsWithLabel(octokit: Octokit) { //: Endpoints["GET /repos/{owner}/{repo}/pulls"]["response"] {
  const { data: pulls } = await octokit.pulls.list({
    ...REPO,
    state: 'open',
  });
  const result: Pull[] = [];
  for (const pull of pulls) {
    if (!pull.labels.find((label) => label.name == 'merge-to:indev')) {
      continue;
    }
    result.push({
      number: pull.number,
      headLabel: pull.head.label,
    });
  }
  return result;
}
