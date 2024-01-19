import { Octokit } from "@octokit/rest";
import * as pull_manager from "./pull_manager";

const octokit = new Octokit({
  auth: process.env.DEPLOY_GITHUB_TOKEN,
});

(async function () {
  const branch = process.env.DEPLOY_BRANCH;
  const owner = process.env.DEPLOY_OWNER;
  const repo = process.env.DEPLOY_REPO;

  const requestedLabel = 'merge-to:' + branch;

  const pulls = await pull_manager.getPullsWithLabel({ owner, repo }, requestedLabel, octokit);

  // numbers list for merge_pulls.sh
  console.log(pulls.map((pull) => pull.number).join(" "));

  // description for Discord message
  console.log("Included PRs:");
  let hasPulls = false;
  for (const pull of pulls) {
    console.log(
      `:arrow_heading_down: `
      + `[${pull.title}](<https://github.com/${owner}/${repo}/pull/${pull.number}>)`
      + ` from ${pull.headLabel} by **${pull.authorLogin}**`
    );
    hasPulls = true;
  }

  if (!hasPulls) {
    console.log("None. Will build clean base branch.")
  }
})().catch((err) => {
  console.error(err)
  process.exit(1);
});
