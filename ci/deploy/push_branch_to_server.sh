#!/usr/bin/env bash

report_fail() {
  ./ci/deploy/call_webhook.sh "Something went wrong, please see GitHub logs for details"
  exit 1
}
trap report_fail ERR

# cheatsheet: https://stackoverflow.com/a/44606194
# ? without : accepts empty string
# Ensuring all required variables are set:
echo "${DEPLOY_TARGET_HOST:?}" > /dev/null
echo "${DEPLOY_TARGET_USER:?}" > /dev/null
echo "${DEPLOY_ACTION:?}" > /dev/null
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

remote_tmp_dir="/tmp/skymp_deploy_`cat /dev/urandom | tr -cd 'a-f0-9' | head -c 32`"
remote_branch_dir="skymp-server-$DEPLOY_BRANCH"

run_remote test -e "$remote_branch_dir" \
  || (echo "no branch on remote server" && exit 1)

rsync --rsh="$remote_shell" -vazPh --checksum \
    ci/deploy/remote/ "$remote_server_connstr:$remote_tmp_dir/"

if [[ "$DEPLOY_ACTION" == "stop" ]]; then
  ./ci/deploy/call_webhook.sh "Stopping the server at \`$remote_server_connstr\`..."
  run_remote "$remote_tmp_dir/branchctl.sh" stop "$DEPLOY_BRANCH"
  ./ci/deploy/call_webhook.sh "Server is now OFFLINE!"
  exit 0
elif [[ "$DEPLOY_ACTION" == "deploy" ]]; then
  ./ci/deploy/call_webhook.sh "Starting deploy of $DEPLOY_BRANCH to \`$remote_server_connstr\`"

  # FIXME(#164): temporary workaround for Chakra build bug
  cp build/vcpkg_installed/x64-linux/bin/libChakraCore.so build/dist/server/
  cp ci/deploy/workaround_temporary/run.sh build/dist/server/

  rsync --rsh="$remote_shell" -vazPh --checksum \
      build/dist/server/ "$remote_server_connstr:$remote_branch_dir/server/"

  ./ci/deploy/call_webhook.sh "Updated server files, restarting it..."
elif [[ "$DEPLOY_ACTION" == "restart" ]]; then
  ./ci/deploy/call_webhook.sh "Restarting server at \`$remote_server_connstr\`..."
else
  ./ci/deploy/call_webhook.sh "Unknown action $DEPLOY_ACTION"
  exit 1
fi

run_remote "$remote_tmp_dir/branchctl.sh" restart "$DEPLOY_BRANCH"

get_ip_port() {
  jq --raw-output '"IP: `" + .ip + "`, port: `" + (.port | tostring) + "`"'
}

ip_port="`run_remote cat "$remote_branch_dir/server-settings.json" | get_ip_port`"

./ci/deploy/call_webhook.sh "Finished successfully. Connect to: $ip_port"
