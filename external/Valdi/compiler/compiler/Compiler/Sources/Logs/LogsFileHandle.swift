//
//  LogsFileHandle.swift
//  
//
//  Created by saniul on 03/12/2021.
//

import Foundation

public protocol LogsFileProtocol {
    var fileURL: URL { get }
    var modificationDate: Date? { get }
    var fileSize: Int { get }
    var fileSizeString: String { get }
}

final class LogsFileHandle: LogsFileProtocol {
    let fileURL: URL

    init(fileURL: URL) {
        self.fileURL = fileURL
    }

    var resourceValues: URLResourceValues? {
        do {
            return try fileURL.resourceValues(forKeys: [.contentModificationDateKey,
                                             .fileSizeKey])
        } catch let error as NSError {
            print("URLResourceValues error: \(error)")
        }
        return nil
    }

    var modificationDate: Date? {
        return resourceValues?.contentModificationDate
    }

    var fileSize: Int {
        return resourceValues?.fileSize ?? 0
    }

    var fileSizeString: String {
        return ByteCountFormatter.string(fromByteCount: Int64(fileSize),
                                         countStyle: .file)
    }
}
