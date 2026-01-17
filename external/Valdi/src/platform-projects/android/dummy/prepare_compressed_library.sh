#!/bin/bash

set -euo pipefail

# import monorepo utils
. "$(dirname ${BASH_SOURCE:-$0})/../../../../utils/monorepo.sh"

REPO_ROOT_DIR="$(get_dev_root)"

export ZSTD="$REPO_ROOT_DIR/src/open_source/tools/zstd/zstdw"

UNPACKED_PATH=$1
pushd $UNPACKED_PATH

# Note: This is renaming XYZ.so to XYZ.zst.so when the file gets
# compressed. This is kind of clowny, but this is prototype code and
# this approach will be revised once functional behavior is verified
# in mushroom.
#
# But why?
#
# The default "XYZ.so.zst" filename used by the zstd tool will get
# ignored by the android gradle MergeNativeLibsTask when it's
# assembling native libs into the final apk, because the filename does
# not end in ".so"
#
# Reference ("predicate" is used to filter only files that match that pattern):
# https://android.googlesource.com/platform/tools/base/+/studio-master-dev/build-system/gradle-core/src/main/java/com/android/build/gradle/internal/tasks/MergeNativeLibsTask.kt#200
#
# One more note is that using this approach will cause some (benign --
# does not fail the build) error messages to show up in the android
# build logs. The build tries to strip symbols from XYZ.zst.so, but
# it's not a format the symbol stripper is expecting.
find jni -name "*.so" -exec \
     bash -c 'set -euo pipefail; export TGT=$(echo $0 | sed "s|.so$|.zst.so|"); ${ZSTD} -f --ultra -22 -o $TGT --rm $0' {} \;
