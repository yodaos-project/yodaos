#!/usr/bin/env bash

set -x

# verify sshkeys
echo $HOME
ls -la ~/.ssh
ssh -tt -o StrictHostKeyChecking=no \
  -i /root/.ssh/id_rsa \
  -p 29418 $ROKID_USERNAME@openai-corp.rokid.com ""

git config --global color.ui false
git config --global user.email "yodaos@rokid.com"
git config --global user.name "yodaos"

repo init \
  -u https://github.com/yodaos-project/yodaos.git \
  -m manifest.xml \
  --repo-url=http://openai-corp.rokid.com/tools/repo \
  --no-repo-verify

repo-username -u $ROKID_USERNAME
repo sync
