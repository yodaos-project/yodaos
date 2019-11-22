#!/usr/bin/env bash
set -e

# Usage:
# $ tools/pull-napi.sh v10.8.0

sed_command=sed
if test $(uname) = 'Darwin'; then
  sed_command=gsed
  if ! type $sed_command &>/dev/null; then
    printf "!! gnu-sed not found. \
      Try install gnu-sed with 'brew install gnu-sed'\n"
    exit 1
  fi
fi

if ! type jq &>/dev/null; then
  printf "!! jq not found. \
    Try install jq with 'brew install jq'\n"
  exit 1
fi

tag=$1

if [[ -z "$tag" ]]; then
  printf "\
  Usage: \n\
$ tools/pull-napi.sh v10.8.0\n"
  exit 1
fi

echo "Fetching Tag information of tag $tag"

tag_sha=$(curl -sL \
  "https://api.github.com/repos/nodejs/node/git/refs/tags/$tag" | \
  jq -r '.object.sha')

if test $tag_sha = 'null'; then
  echo "Tag $tag not found"
  exit 1
fi

work_dir=/tmp/shadow-node
echo "Cleaning up working directory $work_dir, waiting 5s, Ctrl-C to exit"
sleep 5
rm -rf "$work_dir"
mkdir -p "$work_dir"

echo "Fetching Nodejs source codes on tag $tag"
archive_url="https://github.com/nodejs/node/archive/$tag.tar.gz"

arvhice_path="$work_dir/node-$tag.tar.gz"
curl -sLo "$arvhice_path" "$archive_url"
tar -C "$work_dir" -xzf "$arvhice_path"

files_to_pull="\
src/node_api.h;include/node_api.h \
src/node_api_types.h;include/node_api_types.h \
test/addons-napi;test/addons-napi"
for file in $files_to_pull; do
  file_arr=(${file//;/ })
  src="${file_arr[0]}"
  dest="${file_arr[1]}"
  echo "Generating file/directory $dest"

  rm -rf "$dest"
  cp -r "$work_dir/node-${tag//v/}/$src" "$dest"
done

# Prepend a git hash to N-API headers
for file in node_api.h node_api_types.h; do
  command $sed_command -i \
    "1s/^/\
\/\/ Pulled from nodejs\/node#$tag_sha $tag using tools\/pull-napi.sh\n\n/" \
    "include/$file"
done

# Add a description to N-API tests
echo "Pulled from nodejs/node#$tag_sha $tag using tools/pull-napi.sh" \
  > test/addons-napi/README.md

# Alter N-API tests removing Node specific functions
for file in test/addons-napi/**/*.js; do
  [ -f "$file" ] && echo "$file"

  declare -a exps=(
    # `const ` => `var `
    "s/const /var /"
    # `let ` => `var `
    "s/let /var /"
    # `${common.buildType}` => `Release`
    's/\$\{common\.buildType\}/Release/'
    # `./build/Release/binding` => `./build/Release/binding.node`
    "s/(\.\/build\/Release\/\w+)/\1.node/"
  )
  for exp in "${exps[@]}"; do
    command $sed_command -i -r "$exp" "$file"
  done
done
