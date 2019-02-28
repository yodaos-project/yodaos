CWD=`pwd`
REPO=$CWD/tools/repo

mkdir -p yodaos-source
cd yodaos-source

git config --global user.email "circleci@rokid.com"
git config --global user.name "circle ci"

$REPO init \
  -u https://github.com/yodaos-project/yodaos.git \
  -m manifest.xml \
  --repo-url=http://openai-corp.rokid.com/tools/repo \
  --no-repo-verify
