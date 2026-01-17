#!/usr/bin/env python3

import argparse
import os
import subprocess
from sys import platform

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
PROTO_CONFIG_NAME = 'proto_config.yaml'
MODULE_OUT_NAME = 'proto.js'
DECLARATION_OUT_NAME = 'proto.d.ts'
BIN_OUT_NAME = 'proto.protodecl'
MODULE_NAME = 'valdi_protobuf'

SCRIPTS_DIR = os.path.join(SCRIPT_DIR, '../../../../scripts')
JS_V2_DIR = os.path.join(SCRIPTS_DIR, 'protobuf-native')
JS_V2_INDEX_PATH = os.path.join(JS_V2_DIR, 'src/index.ts')

PROTO_DIR = os.path.join(SCRIPT_DIR, 'snap_protos/src/proto')

PROTOC_BIN_PATH = os.path.join(SCRIPT_DIR, '../../../tools/protoc')

def main():
    parser = argparse.ArgumentParser()
    args = parser.parse_args()

    module_path = os.path.join(SCRIPT_DIR, '..')
    filter_config_path = os.path.join(module_path, PROTO_CONFIG_NAME)
    module_out = os.path.join(module_path, 'test', MODULE_OUT_NAME)
    declaration_out = os.path.join(module_path, 'test', DECLARATION_OUT_NAME)
    bin_out = os.path.join(module_path, 'test', BIN_OUT_NAME)
    bin_relative_path = os.path.join(MODULE_NAME, 'test', BIN_OUT_NAME)

    if not os.path.isfile(filter_config_path):
        print('No proto config found for module "{}"'.format(args.module))
        print('Add "{}" to the module root directory to enable proto generation'.format(PROTO_CONFIG_NAME))
        return

    js_dir = JS_V2_DIR
    js_index_path = JS_V2_INDEX_PATH
    node = os.path.join(JS_V2_DIR, 'node_modules', '.bin', 'ts-node')

    subprocess.call(['npm', '--prefix', js_dir, 'install', js_dir])
    subprocess.call([node, js_index_path, '--protoc-bin', PROTOC_BIN_PATH, '--proto-dir', PROTO_DIR, '--filter-config',
                      filter_config_path, '--module-out', module_out, '--declaration-out', declaration_out,
                      '--bin-out', bin_out, '--bin-relative-path', bin_relative_path])
    print('Wrote "{}"'.format(os.path.relpath(module_out)))
    print('Wrote "{}"'.format(os.path.relpath(declaration_out)))
    print('Wrote "{}"'.format(os.path.relpath(bin_out)))

if __name__ == '__main__':
    main()
