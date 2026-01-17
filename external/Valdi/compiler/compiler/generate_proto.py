#!/usr/bin/python

from subprocess import check_call
import os

PROTO_PATH = '.'
INPUT_DIR = os.path.join(PROTO_PATH, '.')
INPUT_FILES = ['valdi', 'valdi-daemon-registry']
SWIFT_FILES = INPUT_FILES + ['valdi-artifact-management']
OUTPUT_SWIFT = 'Compiler/Sources/Models'
OUTPUT_CPP = '../client/src/valdi/protogen-lite/valdi/'

for filename in INPUT_FILES:
    # cpp uses protobuf 3.5.1
    check_call([
        '../client/tools/protoc',
        '--proto_path={}'.format(PROTO_PATH),
        '--cpp_out=lite:{}'.format(OUTPUT_CPP),
        '{}/{}.proto'.format(INPUT_DIR, filename),
    ])

for filename in SWIFT_FILES:
    check_call(['protoc',
        '--swift_out={}'.format(OUTPUT_SWIFT),
        '--proto_path={}'.format(INPUT_DIR),
        '{}/{}.proto'.format(INPUT_DIR, filename)]
    )
