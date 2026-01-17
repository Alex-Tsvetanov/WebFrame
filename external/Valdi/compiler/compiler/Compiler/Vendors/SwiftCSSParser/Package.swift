// swift-tools-version:5.0
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "SwiftCSSParser",
    products: [
        .library(name: "SwiftCSSParser", targets: ["SwiftCSSParser"])
    ],
    dependencies: [
        .package(path: "../katana-parser")
    ],
    targets: [
        .target(
            name: "SwiftCSSParser",
            dependencies: [
                "KatanaParser"
            ],
            path: "Sources")
    ]
)