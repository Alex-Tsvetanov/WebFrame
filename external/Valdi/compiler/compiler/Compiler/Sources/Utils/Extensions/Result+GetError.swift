//
//  Result+GetError.swift
//  Compiler
//
//  Created by saniul on 07/10/2019.
//

import Foundation

extension Swift.Result {
    var success: Success? {
        switch self {
        case let .success(success): return success
        case .failure: return nil
        }
    }

    var failure: Failure? {
        switch self {
        case .success: return nil
        case let .failure(failure): return failure
        }
    }
}
