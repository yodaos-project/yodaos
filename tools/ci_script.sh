CWD=`pwd`
REPO=$CWD/tools/repo
REPO_USERNAME=$CWD/tools/repo-username

cat ~/.ssh/config
cat ~/.ssh/known_hosts

mkdir -p yodaos-source
cd yodaos-source

git config --global user.email "circleci@rokid.com"
git config --global user.name "circle ci"

$REPO init \
  -u https://github.com/yodaos-project/yodaos.git \
  -m manifest.xml \
  --repo-url=http://openai-corp.rokid.com/tools/repo \
  --no-repo-verify

$REPO_USERNAME -u $ROKID_USERNAME
$REPO sync