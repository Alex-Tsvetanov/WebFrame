//
//  CompilationPipeline.swift
//  Compiler
//
//  Created by Simon Corsin on 7/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

final class CompilationPipelineErrorDumper {
    private let deferredWarningCollector: DeferredWarningCollector
    
    init(deferredWarningCollector: DeferredWarningCollector) {
        self.deferredWarningCollector = deferredWarningCollector
    }
    
    func dump(logger: ILogger, error: Error) {
        // NOTE:
        // We are parsing these error messages in our pybuild scripts.
        // If you change how these error messages are formatted, that might
        // require a change in client/snapci/...
        logger.error("=== Start error output ===")
        
        deferredWarningCollector.collectAndPrintAllWarnings()
        
        logger.error("🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑")
        logger.error("🛑                                     🛑")
        logger.error("🛑            Printing errors:         🛑")
        logger.error("🛑                                     🛑")
        logger.error("🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑🛑")
        
        if let compilationPipelineError = error as? CompilationPipelineError {
            let processor = compilationPipelineError.processor
            
            switch compilationPipelineError.kind {
            case .unitemizedError(let error):
                logger.error("Got an error during phase '\(processor.description)': \(error.legibleLocalizedDescription)")
            case .itemizedErrors(let errorItems):
                logger.error("Got \(errorItems.count) \(errorItems.count == 1 ? "error" : "errors") during phase '\(processor.description)':")
                for (item, error) in errorItems {
                    let location = item.relativeProjectPath
                    logger.error("-- \(location): \(error.legibleLocalizedDescription)")
                }
            }
        } else {
            logger.error("Caught global error: \(error.legibleLocalizedDescription)")
        }
        
        logger.error("========================")
        logger.error("=== End error output ===")
        
    }
}

struct CompilationPipelineError: Error {
    enum Kind {
        case unitemizedError(Error)
        case itemizedErrors([(item: CompilationItem, error: Error)])
    }
    
    let processor: CompilationProcessor
    let kind: Kind
    
    init(processor: CompilationProcessor, unitemizedError: Error) {
        self.processor = processor
        self.kind = .unitemizedError(unitemizedError)
    }
    
    init(processor: CompilationProcessor, itemizedErrors: [(item: CompilationItem, error: Error)]) {
        self.processor = processor
        self.kind = .itemizedErrors(itemizedErrors)
    }
}

class CompilationPipeline {
    
    struct Result {
        let warnings: [DeferredWarning]
        let failedItems: [URL: CompilationItem]
    }
    
    private let logger: ILogger
    private let processors: [CompilationProcessor]
    private let deferredWarningCollector: DeferredWarningCollector
    private let failImmediatelyOnError: Bool
    private var onTeardownCallbacks = [() -> Void]()

    init(logger: ILogger, processors: [CompilationProcessor], deferredWarningCollector: DeferredWarningCollector, failImmediatelyOnError: Bool) {
        self.logger = logger
        self.processors = processors
        self.deferredWarningCollector = deferredWarningCollector
        self.failImmediatelyOnError = failImmediatelyOnError
    }

    deinit {
        self.onTeardownCallbacks.forEach { cb in
            cb()
        }
    }

    func onTeardown(_ onTeardownCallback: @escaping () -> Void) {
        self.onTeardownCallbacks.append(onTeardownCallback)
    }

    func process(items: CompilationItems) throws -> Result {
        let mainTransaction = SentryClient.startTransaction(name: "Process", operation: "Running all configured processors")
        
        do {
            deferredWarningCollector.compilationPassStarted()
            defer {
                deferredWarningCollector.compilationPassFinished()
            }
            
            var currentItems = items
            
            var failedItems = [URL: CompilationItem]()
            var deferredWarnings = [DeferredWarning]()
            
            var timerResults = [String]()
            
            let startTime = AbsoluteTime.current
                    
            for processor in processors {
                let time = AbsoluteTime.current
                logger.info("\(processor.description)")
                
                let childTransaction = SentryClient.startChild(parent: mainTransaction, operation: String(describing: type(of: processor)), description: processor.description)
                
                do {
                    currentItems = try processor.process(items: currentItems)
                } catch {
                    SentryClient.finish(transaction: childTransaction)
                    throw CompilationPipelineError(processor: processor, unitemizedError: error)
                }
                
                let errorItems = currentItems.allItems.compactMap { item -> (item: CompilationItem, error: Error)? in
                    if case .error(let error, _) = item.kind {
                        return (item, error)
                    } else {
                        return nil
                    }
                }.sorted { (lhs, rhs) -> Bool in
                    return lhs.item.relativeProjectPath < rhs.item.relativeProjectPath
                }
                
                if errorItems.count > 0 {
                    if failImmediatelyOnError {
                        throw CompilationPipelineError(processor: processor, itemizedErrors: errorItems)
                    } else {
                        for (item, _) in errorItems {
                            failedItems[item.sourceURL] = item
                        }
                    }
                }
                
                deferredWarnings.append(contentsOf: deferredWarningCollector.collectWarnings())
                
                SentryClient.finish(transaction: childTransaction)
                timerResults.append("-- \(processor.description): \(time.timeElapsed.milliseconds)ms")
            }
            
            SentryClient.finish(transaction: mainTransaction)
            logger.info("""
                Compilation took \(startTime.timeElapsed.milliseconds)ms
                \(timerResults.joined(separator: "\n"))
                """)
            
            return Result(warnings: deferredWarnings, failedItems: failedItems)
        } catch {
            SentryClient.finish(transaction: mainTransaction)
            throw error
        }
    }
}
