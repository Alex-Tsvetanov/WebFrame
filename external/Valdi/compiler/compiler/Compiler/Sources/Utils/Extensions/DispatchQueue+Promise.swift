//
//  DispatchQueue+Promise.swift
//  Compiler
//
//  Created by Simon Corsin on 12/11/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

extension DispatchQueue {

    func asyncPromise<T>(_ closure: @escaping () -> Promise<T>) -> Promise<T> {
        let promise = Promise<T>()

        async {
            _ = closure().then { (data) -> Void in
                promise.resolve(data: data)
                }.catch { (error) in
                    promise.reject(error: error)
                }
        }

        return promise
    }

    func asyncPromise<T>(_ closure: @escaping () throws -> T) -> Promise<T> {
        let promise = Promise<T>()

        async {
            do {
                let result = try closure()
                promise.resolve(data: result)
            } catch let error {
                promise.reject(error: error)
            }
        }

        return promise
    }

}
