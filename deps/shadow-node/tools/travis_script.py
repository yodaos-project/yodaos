#!/usr/bin/env python

import os

from common_py.system.executor import Executor as ex


BUILDTYPES = ['debug', 'release']


def check_change(path):
    '''Check if current pull request depends on path,
    return `False` if not depends, else if depends.'''
    if os.getenv('TRAVIS_PULL_REQUEST') == 'false':
        return True
    travis_branch = os.getenv('TRAVIS_BRANCH')
    commit_diff = ex.run_cmd_output('git',
                                    [
                                        'diff',
                                        '--name-only',
                                        'HEAD..origin/' + travis_branch],
                                    True)
    return commit_diff.find(path) != -1


def build_jerry():
    if check_change('deps/jerry'):
        ex.check_run_cmd('./deps/jerry/tools/run-tests.py',
                         [
                             '--quiet',
                             '--unittests',
                             '--jerry-tests',
                             '--jerry-test-suite'
                         ]
                        )


def build_iotjs(buildtype, args=[], env=[]):
    ex.check_run_cmd('./tools/build.py',
                     ['--clean', '--buildtype=' + buildtype] + args, env)


if __name__ == '__main__':
    test = os.getenv('OPTS')
    if test == 'host-linux':
        build_jerry()
        for buildtype in BUILDTYPES:
            build_iotjs(buildtype, [
                '--run-test=full',
                '--no-check-valgrind'])

    elif test == "host-darwin":
        for buildtype in BUILDTYPES:
            build_iotjs(buildtype, [
                '--run-test=full',
                '--no-check-valgrind',
                '--profile=test/profiles/host-darwin.profile'])

    elif test == 'rpi2':
        for buildtype in BUILDTYPES:
            build_iotjs(buildtype, [
                        '--target-arch=arm',
                        '--target-board=rpi2',
                        '--profile=test/profiles/rpi2-linux.profile'])

    elif test == "no-snapshot":
        for buildtype in BUILDTYPES:
            build_iotjs(buildtype, [
                        '--run-test=full',
                        '--no-check-valgrind',
                        '--no-snapshot',
                        '--jerry-lto'])

    elif test == 'napi':
        for buildtype in BUILDTYPES:
            build_iotjs(buildtype, [
                '--run-test=full',
                '--no-check-valgrind',
                '--napi'])

    elif test == "coverity":
        ex.check_run_cmd('./tools/build.py', ['--clean'])
