//
//  Result+FromTuple.swift
//  Compiler
//
//  Created by saniul on 26/06/2019.
//

import Foundation

// Sometimes we have to deal with situations where the success or failure is expressed
// as a pair of optional objects.
// This utility extension exists to convert that easily into a Swift.Result instance.
extension Result where Failure == Error {
    init(fromTuple tuple: (Success?, Failure?)) {
        switch tuple {
        case (.none, .none), (.some, .some):
            self = .failure(CompilerError("Invalid result \(tuple)"))
        case let (.none, .some(failure)):
            self = .failure(failure)
        case let (.some(success), .none):
            self = .success(success)
        }
    }
}
