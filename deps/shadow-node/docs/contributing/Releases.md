# ShadowNode Release Process

This document describes the technical aspects of the ShadowNode release process.

## Table of Contents

* [How to create a release](#how-to-create-a-release)
  * [0. Pre-release steps](#0-pre-release-steps)
  * [1. Update the staging branch](#1-update-the-staging-branch)
  * [2. Create a new branch for the release](#2-create-a-new-branch-for-the-release)
  * [3. Update version code](#3-update-version-code)
  * [4. Update the Changelog](#4-update-the-changelog)
  * [5. Create Release Commit](#5-create-release-commit)
  * [6. Propose Release on GitHub](#6-propose-release-on-github)
  * [7. Test the Build](#7-test-the-build)
  * [8. Tag and Sign the Release Commit](#8-tag-and-sign-the-release-commit)
  * [9. Set Up For the Next Release](#9-set-up-for-the-next-release)
  * [10. Check the Release](#10-check-the-release)
  * [11. Create the release on GitHub](#11-create-the-release-on-github)
  * [12. Cleanup](#12-cleanup)
  * [13. Celebrate](#13-celebrate)

## How to create a release

Notes:

- Dates listed below as _"YYYY-MM-DD"_ should be the date of the release **as
  UTC**. Use `date -u +'%Y-%m-%d'` to find out what this is.
- Version strings are listed below as _"vx.y.z"_ or _"x.y.z"_. Substitute for
  the release version.
- Examples will use the fictional release version `1.2.3`.

### 0. Pre-release steps

Before preparing a ShadowNode release, the Build Working Group must be notified at
least one business day in advance of the expected release. Coordinating with
Build is essential to make sure that the CI works, release files are published,
and the release blog post is available on the project website.

### 1. Update the staging branch

Checkout the staging branch locally.

```console
$ git remote update
$ git checkout v1.x-staging
$ git reset --hard upstream/v1.x-staging
```

If the staging branch is not up to date relative to `master`, bring the
appropriate PRs and commits into it.

For each PR:
- Run or check that there is a passing CI.
- Check approvals (you can approve yourself).
- Check that the commit metadata was not changed from the `master` commit.
- If there are merge conflicts, ask the PR author to rebase.
Simple conflicts can be resolved when landing.

When landing the PR add the `Backport-PR-URL:` line to each commit. Update
the label on the original PR from `backport-requested-vN.x` to `backported-to-vN.x`.

For a list of commits that could be landed in a patch release on v1.x:

```console
$ branch-diff v1.x-staging master --exclude-label=semver-major,semver-minor,dont-land-on-v1.x,backport-requested-v1.x --filter-release --format=simple
```

Previous release commits and version bumps do not need to be
cherry-picked.

Carefully review the list of commits:
- Checking for errors (incorrect `PR-URL`)
- Checking semver status - Commits labeled as `semver-minor` or `semver-major`
should only be cherry-picked when appropriate for the type of release being
made.
- If you think it's risky so should wait for a while, add the `baking-for-lts`
   tag.

When cherry-picking commits, if there are simple conflicts you can resolve
them. Otherwise, add the `backport-requested-vN.x` label to the original PR
and post a comment stating that it does not land cleanly and will require a
backport PR.

If commits were cherry-picked in this step, check that the test still pass and
push to the staging branch to keep it up-to-date.

```console
$ git push upstream v1.x-staging
```

### 2. Create a new branch for the release

Create a new branch named `vx.y.z-proposal`, off the corresponding staging
branch.

```console
$ git checkout -b v1.2.3-proposal upstream/v1.x-staging
```

### 3. Update version code

Set the version for the proposed release using the following macros, which are
already defined in `CMakeList.txt`:

```cmake
set(IOTJS_VERSION_MAJOR x)
set(IOTJS_VERSION_MINOR y)
set(IOTJS_VERSION_PATCH z)
```

Also version in `package.json` should be updated:

```json
  "version": "x.y.z"
```

### 4. Update the Changelog

#### Step 1: Collect the formatted list of changes

Collect a formatted list of commits since the last release. Use
[`generate-changelog`](https://github.com/yodaos-project/yoda-core-util/blob/master/bin/generate-changelog) to do this:

```console
$ generate-changelog --since v0.11.5 --version v0.11.6
```

### 5. Create Release Commit

The `src/node_version.h` changes should be the final commit that will be
tagged for the release. When committing these to git, use the following message format:

```txt
YYYY-MM-DD, Version x.y.z (Release Type)

Notable changes:

* Copy the notable changes list here, reformatted for plain-text
```


### 6. Propose Release on GitHub

Push the release branch to `yodaos-project/ShadowNode`, not to your own fork. This
allows release branches to more easily be passed between members of the release team
if necessary.

Create a pull request targeting the correct release line. For example, a
`v5.3.0-proposal` PR should target `v5.x`, not master. Paste the CHANGELOG
modifications into the body of the PR so that collaborators can see what is
changing. These PRs should be able to be updated as new commits land.

If you need any additional information about any of the commits, this PR is a
good place to @-mention the relevant contributors.

### 7. Test the Build

Jenkins collects the artifacts from the builds, allowing you to download and
install the new build. Make sure that the build appears correct. Check the
version numbers, and perform some basic checks to confirm that all is well with
the build before moving forward.

### 8. Tag and Sign the Release Commit

Once you have produced builds that you're happy with, create a new tag. By
waiting until this stage to create tags, you can discard a proposed release if
something goes wrong or additional commits are required. Once you have created a
tag and pushed it to GitHub, you ***must not*** delete and re-tag. If you make
a mistake after tagging then you'll have to version-bump and start again and
count that tag/version as lost.

Tag summaries have a predictable format, look at a recent tag to see, `git tag
-v v6.0.0`. The message should look something like `2016-04-26 ShadowNode v6.0.0
(Current) Release`.

Install `git-secure-tag` npm module:

```console
$ npm install -g git-secure-tag
```

Create a tag using the following command:

```console
$ git secure-tag <vx.y.z> <commit-sha> -sm 'YYYY-MM-DD ShadowNode vx.y.z (Release Type) Release'
```

The tag **must** be signed using the GPG key that's listed for you on the
project README.

Push the tag to the repo before you promote the builds. If you haven't pushed
your tag first, then build promotion won't work properly. Push the tag using the
following command:

```console
$ git push <remote> <vx.y.z>
```

*Note*: Please do not push the tag unless you are ready to complete the
remainder of the release steps.

### 9. Set Up For the Next Release

On release proposal branch, edit `src/node_version.h` again and:

- Increment `NODE_PATCH_VERSION` by one
- Change `NODE_VERSION_IS_RELEASE` back to `0`

Commit this change with the following commit message format:

```txt
Working on vx.y.z # where 'z' is the incremented patch number

PR-URL: <full URL to your release proposal PR>
```

This sets up the branch so that nightly builds are produced with the next
version number _and_ a pre-release tag.

Merge your release proposal branch into the stable branch that you are releasing
from and rebase the corresponding staging branch on top of that.

```console
$ git checkout v1.x
$ git merge --ff-only v1.2.3-proposal
$ git push upstream v1.x
$ git checkout v1.x-staging
$ git rebase v1.x
$ git push upstream v1.x-staging
```

Cherry-pick the release commit to `master`. After cherry-picking, edit
`src/node_version.h` to ensure the version macros contain whatever values were
previously on `master`. `NODE_VERSION_IS_RELEASE` should be `0`. **Do not**
cherry-pick the "Working on vx.y.z" commit to `master`.

Run `make lint` before pushing to `master`, to make sure the Changelog
formatting passes the lint rules on `master`.

### 10. Check the Release

Your release should be available at `https://github.com/yodaos-project/ShadowNode/releases/tag/vx.y.z`.
Check that the appropriate files are in place. You may want to check that the binaries
are working as appropriate and have the right internal version strings.

### 11. Create the release on GitHub

- Go to the [New release page](https://github.com/yodaos-project/ShadowNode/releases/new).
- Select the tag version you pushed earlier.
- For release title, copy the title from the changelog.
- For the description, copy the rest of the changelog entry.
- Click on the "Publish release" button.

### 12. Cleanup

Close your release proposal PR and delete the proposal branch.

### 13. Celebrate

_In whatever form you do this..._
