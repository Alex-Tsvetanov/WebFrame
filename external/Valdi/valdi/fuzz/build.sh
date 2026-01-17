#!/bin/bash -eu
set +x

: "${BAZEL_FUZZ_TEST_TAG:=fuzz-test}"
: "${BAZEL_FUZZ_TEST_EXCLUDE_TAG:=no_oss_fuzz}"
: "${BAZEL_PACKAGE_SUFFIX:=_oss_fuzz}"
: "${BAZEL_TOOL:=bzl}"
: "${BAZEL_EXTRA_BUILD_FLAGS:=}"

BAZEL_LANGUAGE=cc


if [[ -z "${BAZEL_FUZZ_TEST_QUERY:-}" ]]; then
    BAZEL_FUZZ_TEST_QUERY="
        let all_fuzz_tests = attr(tags, \"${BAZEL_FUZZ_TEST_TAG}\", \"@valdi//valdi/...\") in
        let lang_fuzz_tests = attr(generator_function, \"^${BAZEL_LANGUAGE}_fuzz_test\$\", \$all_fuzz_tests) in
        \$lang_fuzz_tests - attr(tags, \"${BAZEL_FUZZ_TEST_EXCLUDE_TAG}\", \$lang_fuzz_tests)
    "
fi

echo "Using Bazel query to find fuzz targets: ${BAZEL_FUZZ_TEST_QUERY}"

declare -r FUZZ_TESTS=(
    $(${BAZEL_TOOL} query "${BAZEL_FUZZ_TEST_QUERY}" | sed "s/$/${BAZEL_PACKAGE_SUFFIX}/")
)

if [[ ${#FUZZ_TESTS[@]} -eq 0 ]]; then
    echo "Expected at least one fuzz test package."
    exit 1
fi

echo "Found ${#FUZZ_TESTS[@]} fuzz test packages:"
for fuzz_test in "${FUZZ_TESTS[@]}"; do
    echo "  ${fuzz_test}"
done

declare -r EXTRA_BAZEL_FLAGS="$(
if [ "$SANITIZER" = "undefined" ]
then
    echo "--config=ubsan-libfuzzer"
elif [ "$SANITIZER" = "address" ]
then
    echo "--config=asan-libfuzzer"
fi
)"

declare -r BAZEL_BUILD_FLAGS=(
    "-c" "opt" \
    "--copt=-g" \
    "--strip=never" \
    "--verbose_failures" \
    ${BAZEL_EXTRA_BUILD_FLAGS[*]} \
    ${EXTRA_BAZEL_FLAGS}
)

echo "Building the fuzz tests with the following Bazel options:"
echo "  ${BAZEL_BUILD_FLAGS[@]}"

echo "Extra bazel flags:"
echo "  ${EXTRA_BAZEL_FLAGS}"

echo "running command:"
echo "${BAZEL_TOOL} build ${BAZEL_BUILD_FLAGS[@]} ${FUZZ_TESTS[@]}"

${BAZEL_TOOL} build "${BAZEL_BUILD_FLAGS[@]}" "${FUZZ_TESTS[@]}"

echo "Extracting the fuzz test packages in the output directory."
for fuzz_archive in $(find bazel-bin/ -name "*${BAZEL_PACKAGE_SUFFIX}.tar"); do
    tar -xvf "${fuzz_archive}" -C "${OUT}"
    echo "Cleaning up fuzz test archive at ${fuzz_archive}"
    rm "${fuzz_archive}"
done
