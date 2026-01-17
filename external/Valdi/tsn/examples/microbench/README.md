## Build and run the benchmark app (in interpreted mode)

```
bzl build //tsn/examples:microbench && bazel-bin/tsn/examples/microbench "tsn/examples/microbench/microbench.js"
```

## Run and save the result as baseline

```
bzl build //tsn/examples:microbench && bazel-bin/tsn/examples/microbench "tsn/examples/microbench/microbench.js -s baseline.txt"
```

## Run and use a baseline file as reference

```
bzl build //tsn/examples:microbench && bazel-bin/tsn/examples/microbench "tsn/examples/microbench/microbench.js -r baseline.txt"
```

## Run specific tests and compare to a baseline file

```
bzl build //tsn/examples:microbench && bazel-bin/tsn/examples/microbench "tsn/examples/microbench/microbench.js prop_create prop_update -r baseline.txt"
```

## Run all tests start with "prop" and compare to a baseline file

```
bzl build //tsn/examples:microbench && bazel-bin/tsn/examples/microbench "tsn/examples/microbench/microbench.js prop -r baseline.txt"
```
