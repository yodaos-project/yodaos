#!/usr/bin/env bash

set -x

chown -R node:$HOST_UID /opt/codebase
ls -la /home/

# verify sshkeys
ssh -tt -o StrictHostKeyChecking=no -p 29418 $ROKID_USERNAME@openai-corp.rokid.com ""

git config --global color.ui false
git config --global user.email "yodaos@rokid.com"
git config --global user.name "yodaos"

repo init \
  -u https://github.com/yodaos-project/yodaos.git \
  -m manifest.xml \
  --repo-url=http://openai-corp.rokid.com/tools/repo \
  --no-repo-verify

ls -la ./
repo-username -u $ROKID_USERNAME
repo sync
