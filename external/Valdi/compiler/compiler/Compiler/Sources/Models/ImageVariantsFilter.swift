// Copyright © 2025 Snap, Inc. All rights reserved.

struct ImageVariantsFilter: Decodable {

    let scalesByPlatform: [Platform: [Double]]

    init(scalesByPlatform: [Platform: [Double]]) {
        self.scalesByPlatform = scalesByPlatform
    }

    func shouldInclude(platform: Platform, scale: Double) -> Bool {
        guard let scales = scalesByPlatform[platform] else {
            return true
        }

        return scales.contains(where: { $0 == scale })
    }

    static func parse(str: String) throws -> ImageVariantsFilter {
        // example: android=2.0,3.0;ios=2.0
        var scalesByPlatform = [Platform: [Double]]()
        let components = str.split(separator: ";")

        for component in components {
            let nested = component.split(separator: "=")

            if nested.count != 2 {
                throw CompilerError("Invalid image variants filter")
            }

            guard let platform = Platform(rawValue: String(nested[0])) else {
                throw CompilerError("Invalid platform '\(nested[0])'")
            }

            let unparsedScales = nested[1].split(separator: ",")

            let scales = try unparsedScales.map {
                guard let scale = Double($0) else {
                    throw CompilerError("Invalid scale '\($0)'")
                }
                return scale
            }

            scalesByPlatform[platform] = scales
        }

        return ImageVariantsFilter(scalesByPlatform: scalesByPlatform)
    }

}
