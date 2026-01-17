"""
Clients can put their own compile flags here if desired.

Select statements are supported.
"""

DEFAULT_COPTS = [
    "-fstrict-aliasing",
    "-fvisibility=hidden",
    "-std=c++17",
    "-Wno-psabi",
    "-Wno-import-preprocessor-directive-pedantic",
    "-Wno-shorten-64-to-32",
    "-Wno-unguarded-availability-new",
] + select({
    # Turning off RTTI reduces code size, but is necessary for connecting C++
    # and Objective-C code.
    "@platforms//os:macos": [],
    "@platforms//os:ios": [],
    "//conditions:default": [
        "-fno-rtti",
    ],
})

DEFAULT_OBJC_COPTS = [
    "-Wno-shorten-64-to-32",
    "-Wno-deprecated-declarations",
    "-Wno-unguarded-availability-new",
]
