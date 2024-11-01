#!/usr/bin/env bash

# cheatsheet: https://stackoverflow.com/a/44606194
# ? without : accepts empty string
# Ensuring all required variables are set:
echo "${DEPLOY_TARGET_HOST:?}" > /dev/null
echo "${DEPLOY_TARGET_USER:?}" > /dev/null
echo "${GREP_ARG?}" > /dev/null # OK to be empty
echo "${TAIL_ARG:?}" > /dev/null
echo "${DEPLOY_BRANCH:?}" > /dev/null
echo "${DEPLOY_SSH_PRIVATE_KEY:?}" > /dev/null
echo "${DEPLOY_SSH_KNOWN_HOSTS:?}" > /dev/null

if [[ "$CI" != "" ]]; then
  sudo chmod -R 777 .
fi

touch ssh_id
touch ssh_known_hosts
chmod 600 ssh_id
chmod 600 ssh_known_hosts
echo "$DEPLOY_SSH_PRIVATE_KEY" > ssh_id
echo "$DEPLOY_SSH_KNOWN_HOSTS" > ssh_known_hosts

remote_shell="ssh -i ssh_id -o UserKnownHostsFile=ssh_known_hosts"
remote_server_connstr="$DEPLOY_TARGET_USER@$DEPLOY_TARGET_HOST"

run_remote() {
  $remote_shell "$remote_server_connstr" "$@"
}

remote_branch_dir="skymp-server-$DEPLOY_BRANCH"

run_remote test -e "$remote_branch_dir" \
  || (echo "no branch on remote server" && exit 1)

run_remote "bash -c 'docker logs --tail $TAIL_ARG \"$remote_branch_dir\" | grep \"$GREP_ARG\"'"
