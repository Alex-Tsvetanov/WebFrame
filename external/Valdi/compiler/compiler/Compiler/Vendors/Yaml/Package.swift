// swift-tools-version:5.0
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "Yaml",
    products: [
        .library(name: "Yaml", targets: ["Yaml"])
    ],
    dependencies: [],
    targets: [
        .target(
            name: "Yaml",
            path: "Sources")
    ]
)