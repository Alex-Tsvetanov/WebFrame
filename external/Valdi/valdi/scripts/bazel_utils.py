#!/usr/bin/python

import subprocess
import os


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, '../../..'))


def __run_bazel(request, parameters):
    resolved_parameters = ['bzl', request] + parameters
    print('Running: {}'.format(' '.join(resolved_parameters)))
    return subprocess.check_output(resolved_parameters).strip()


def build(bazel_targets, additional_options):
    if not isinstance(bazel_targets, list):
        bazel_targets = [bazel_targets]

    __run_bazel('build', bazel_targets + additional_options)


def get_output(target):
    if not target.startswith('//'):
        raise Exception('Only absolute targets are supported')

    return os.path.join(ROOT_DIR, 'bazel-bin/', target[2:].replace(':', '/'))


def get_release_build_options():
    return ['--snap_flavor=production', '--define=dynamic_cronet=true']


def get_debug_build_options():
    return ['--snap_flavor=platform_development', '--define=dynamic_cronet=true']
