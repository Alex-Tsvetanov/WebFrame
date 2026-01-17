// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation

struct CompilationMetadata: Codable {

    let classMappings: [String: ValdiClass]
    let nativeTypes: SerializedTypeScriptNativeTypeResolver

}
