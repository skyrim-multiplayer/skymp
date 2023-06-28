import { Octokit } from "@octokit/rest";
import * as pull_manager from "./pull_manager";

const octokit = new Octokit();

(async function () {
  const pulls = await pull_manager.getPullsWithLabel(octokit);

  // numbers list for merge_pulls.sh
  console.log(pulls.map((pull) => pull.number).join(" "));

  // description for Discord message
  console.log("Included PRs:");
  let hasPulls = false;
  for (const pull of pulls) {
    console.log(
      `:arrow_heading_down: `
      + `[${pull.title}](<https://github.com/skyrim-multiplayer/skymp/pull/${pull.number}>)`
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
