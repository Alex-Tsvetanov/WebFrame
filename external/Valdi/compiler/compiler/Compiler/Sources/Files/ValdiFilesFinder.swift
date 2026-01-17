//
//  CompserFilesFinder.swift
//  Compiler
//
//  Created by Simon Corsin on 4/7/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

protocol ValdiFilesFinderDelegate: AnyObject {

    func valdiFilesFinder(_ valdiFilesFinder: ValdiFilesFinder, filesDidChange files: [URL])

}

class ValdiFilesFinder {

    weak var delegate: ValdiFilesFinderDelegate?

    private let queue = DispatchQueue(label: "com.snap.valdi.filesfinder")
    private var foundFileURLs: Set<URL> = []
    private var watchmanClient: WatchmanClient?

    private let urls: [URL]

    private var monitoringObserver: Cancelable?

    init(urls: [URL]) {
        self.urls = urls
    }

    convenience init(url: URL) {
        self.init(urls: [url])
    }

    func startMonitoring(logger: ILogger) throws {
        guard watchmanClient == nil else { return }

        let watchmanClient = try WatchmanClient(logger: logger)
        self.watchmanClient = watchmanClient

        let watchmanRoot = URL.commonFileURL(in: self.urls)

        let watchedPaths: [String] = self.urls.map { RelativePath(from: watchmanRoot, to: $0).description }

        monitoringObserver = watchmanClient.subscribe(projectPath: watchmanRoot.path, paths: watchedPaths) { [weak self] changes in
            self?.queue.async {
                guard let strongSelf = self else { return }
                strongSelf.process(notifications: changes)
            }
        }
    }

    func stopMonitoring() {
        monitoringObserver?.cancel()
        monitoringObserver = nil
        watchmanClient = nil
    }

    private func process(notifications: [WatchmanFileNotification]) {
        var updatedFileURLs = [URL]()
        for notification in notifications {
            let fileURL = notification.url

            switch notification.kind {
            case .removed:
                self.foundFileURLs.remove(fileURL)
            case .created:
                fallthrough
            case .changed:
                if shouldUseFile(at: fileURL) {
                    self.foundFileURLs.insert(fileURL)
                    updatedFileURLs.append(fileURL)
                }
            }
        }

        if !updatedFileURLs.isEmpty {
            delegate?.valdiFilesFinder(self, filesDidChange: updatedFileURLs)
        }
    }

    func allFiles() throws -> [URL] {
        // Dedupe since it's possible for a file to be counted twice if it meets multiple queries
        var files = Set<URL>()
        try forEachFile { (file) in
            files.insert(file)
        }
        return Array(files)
    }

    private func forEachFile(_ closure: (URL) -> Void) throws {
        for url in urls {
            try forEachFile(url: url, closure: closure)
        }
    }

    private func forEachFile(url: URL, closure: (URL) -> Void) throws {
        guard let enumerator = FileManager.default.enumerator(at: url, includingPropertiesForKeys: nil, options: []) else {
            throw CompilerError("Failed to get enumerator")
        }

        while let url = (enumerator.nextObject() as? URL)?.standardizedFileURL {
            let realUrl: URL
            do {
                realUrl = try url.resolvingSymlink()
            } catch SymlinkErrors.resolvedSymlinkDoesNotExist {
                continue
            }

            if realUrl.hasDirectoryPath {
                if realUrl != url {
                    try forEachFile(url: realUrl) { (innerUrl) in
                        let relativePath = realUrl.relativePath(to: innerUrl)
                        let relativeUrl = url.appendingPathComponent(relativePath)

                        closure(relativeUrl)
                    }
                }
                continue
            }

            closure(url)
        }
    }

    private func shouldUseFile(at url: URL) -> Bool {
        var isDirectory: ObjCBool = false
        if !FileManager.default.fileExists(atPath: url.path, isDirectory: &isDirectory) {
            return false
        }
        if isDirectory.boolValue {
            // We only care about the files, not the directories
            return false
        }

        return true
    }
}
