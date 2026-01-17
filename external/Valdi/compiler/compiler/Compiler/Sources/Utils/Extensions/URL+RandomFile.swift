//
//  URL+RandomFile.swift
//  Spectre
//
//  Created by Simon Corsin on 3/13/17.
//  Copyright © 2017 Snapchat, Inc. All rights reserved.
//

import Foundation

extension URL {

    static func randomFileURL(extension ext: String? = nil) -> URL {
        let filenameWithoutExtension = UUID().uuidString
        let filename = ext.map { "\(filenameWithoutExtension).\($0)" } ?? filenameWithoutExtension
        return URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent(filename)
    }

}

extension NSURL {

    // Obj-C version
    class func randomFileURL(extension ext: String? = nil) -> NSURL {
        return URL.randomFileURL(extension: ext) as NSURL
    }

}
