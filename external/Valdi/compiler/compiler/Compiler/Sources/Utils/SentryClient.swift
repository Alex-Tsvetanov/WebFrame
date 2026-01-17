//
//  SentryClient.swift
//
// Created by Vasily Fomin on 03/26/24.
// Copyright © 2024 Snap, Inc. All rights reserved.

import Foundation

#if os(macOS) && !DEBUG
import Sentry

// Upstream Sentry doesn't support Linux and we don't want to send events for DEBUG builds.
// This wrapper is to avoid spreading #ifdefs all over the place
class SentryClient {
    private static func getObjCClass(name: String) -> NSObject {
        return name.withCString( { ptr in
            return objc_getClass(ptr)
        }) as! NSObject
    }

    private static func sendObjCMessage(obj: NSObject, message: String) -> NSObject? {
        let selector = NSSelectorFromString(message)
        guard let result = obj.perform(selector) else {
            return nil
        }

        return result.takeUnretainedValue() as? NSObject
    }

    private static func sendObjCMessage(obj: NSObject, message: String, argument: Any?) -> NSObject? {
        let selector = NSSelectorFromString(message)
        guard let result = obj.perform(selector, with: argument) else {
            return nil
        }

        return result.takeUnretainedValue() as? NSObject
    }

    private static func hackSentrySoThatItDoesntCrash(options: Options) {
        autoreleasepool {
            // Inject options into Sentry, so that SentryCrashWrapper.sharedInstance().systemInfo
            // resolves the right options
            let sentryCls = getObjCClass(name: "SentrySDK")
            let _ = sendObjCMessage(obj: sentryCls, message: "setStartOptions:", argument: options)

            // Executes: SentryCrashWrapper.sharedInstance().systemInfo
            // The goal is to workaround a deadlock within Sentry by eagerly initializing
            // SentryCrashWrapper's sharedInstance and its systemInfo property, which
            // uses a global lock
            let cls = getObjCClass(name: "SentryCrashWrapper")
            let sharedInstance = sendObjCMessage(obj: cls, message: "sharedInstance")!
            let _ = sendObjCMessage(obj: sharedInstance, message: "systemInfo")
        }
    }

    static func start(dsn: String, tracesSampleRate: NSNumber, username: String) {
        let options = Options()
        options.dsn = dsn
        options.tracesSampleRate = tracesSampleRate
        options.enableAppHangTracking = false

        hackSentrySoThatItDoesntCrash(options: options)

        SentrySDK.start(options: options)
        
        let user = User()
        user.username = username
        SentrySDK.setUser(user)
    }
    
    static func startTransaction(name: String, operation: String) -> Span {
        return SentrySDK.startTransaction(
            name: name,
            operation: operation
        )
    }
    
    static func startChild(parent: Span, operation: String, description: String) -> Span {
        return parent.startChild(operation: operation, description: description)
        
    }
    
    static func finish(transaction: Span, error: Error? = nil) {
        if let error {
            SentrySDK.capture(error: error)
            transaction.finish(status: .internalError)
        } else {
            transaction.finish()
        }
    }
    
    static func setTag(value: String, key: String) {
        SentrySDK.configureScope { scope in
            scope.setTag(value: value, key: key)
        }
    }
}
#else
// Linux/Debug client - all operations are no-op
class SentryClient {
    static func start(dsn: String, tracesSampleRate: NSNumber, username: String) { }
    static func startTransaction(name: String, operation: String) -> Any? { return nil }
    static func startChild(parent: Any?, operation: String, description: String) -> Any? { return nil }
    static func finish(transaction: Any?, error: Error? = nil) { }
    static func setTag(value: String, key: String) { }
}
#endif
