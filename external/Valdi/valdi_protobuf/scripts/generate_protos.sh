#!/usr/bin/env bash

set -eoux pipefail

# import monorepo utils
. "$(dirname ${BASH_SOURCE:-$0})/../../../../utils/monorepo.sh"

script_root="$(dirname $(realpath "${BASH_SOURCE[0]}"))"
cd "$script_root"
repo_root=$(get_dev_root)

protoc="${repo_root}/tools/protoc"

(
  cd "$script_root/.."
  pwd

  test_root=${repo_root}/src/open_source/valdi_protobuf/test
  gen=${test_root}/protogen

  rm -rf "${gen}"
  mkdir -p "${gen}"

  echo "Generating test protos"

  "${protoc}" --proto_path="${test_root}/proto" \
    --cpp_out="${gen}" \
    "${test_root}/proto/test.proto"

  # Remove generated google protos such as any.pb.*, empty.pb.*
  rm -rf "${gen}/google"

  echo Ok
)
