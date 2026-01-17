#!/usr/bin/env bash

set -eu

script_root="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
repo_root="$(cd "$script_root"/../../../.. && pwd)"

protoc="${repo_root}/tools/protoc"
valdi_root="${repo_root}/src/open_source/valdi"
compiler_root="${repo_root}/src/open_source/compiler"

generate_protos() {
  local proto_path="${compiler_root}/compiler/"

  local cpp_out="${valdi_root}/protogen-lite/valdi"
  local valdi_cpp_proto_files=(
    valdi.proto
  )

  mkdir -p "${cpp_out}"

  ${protoc} --proto_path="${proto_path}" \
    --cpp_out="lite:${cpp_out}" \
    $valdi_cpp_proto_files


  local swift_out="${compiler_root}/compiler/Compiler/Sources/Models"
  local valdi_swift_proto_files=(
    valdi-artifact-management.proto
    valdi-daemon-registry.proto
  )
  ${protoc} --proto_path="${proto_path}" \
    --swift_out="${swift_out}" \
    $valdi_swift_proto_files
}

(
  cd ${repo_root}
  generate_protos
)
