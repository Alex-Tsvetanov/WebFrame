// swift-tools-version:4.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "Zstd",
    products: [
        .library(name: "Zstd", targets: ["Zstd"])
    ],
    dependencies: [],
    targets: [
        .target(
            name: "Zstd",
            path: "Sources")
    ]
)
