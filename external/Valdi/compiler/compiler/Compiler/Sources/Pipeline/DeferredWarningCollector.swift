import Foundation

struct DeferredWarning {
    let tag: String
    let message: String
}

final class DeferredWarningCollector {
    private let reportedWarnings = Synchronized<[DeferredWarning]>(data: [])

    private var shouldDefer = false
    private let logger: ILogger

    init(logger: ILogger) {
        self.logger = logger
    }

    func compilationPassStarted() {
        shouldDefer = true
    }

    func compilationPassFinished() {
        shouldDefer = false
    }

    func reportWarning(_ warning: DeferredWarning) {
        if shouldDefer {
            self.reportedWarnings.data { $0.append(warning) }
        } else {
            logger.error("⚠️  Warning: \(warning.tag): \(warning.message)")
        }
    }

    func collectWarnings() -> [DeferredWarning] {
        let events: [DeferredWarning] = self.reportedWarnings.data { data in
            let copy = data
            data.removeAll()
            return copy
        }
        return events
    }

    func collectAndPrintAllWarnings() {
        let deferredWarnings = collectWarnings()
        guard deferredWarnings.count > 0 else { return }

        logger.error("⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️")
        logger.error("⚠️                                     ⚠️")
        logger.error("⚠️           Printing warnings:        ⚠️")
        logger.error("⚠️                                     ⚠️")
        logger.error("⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️ ⚠️")
        deferredWarnings.forEach {
            logger.error("-- \($0.tag): \($0.message)")
        }
    }

}
