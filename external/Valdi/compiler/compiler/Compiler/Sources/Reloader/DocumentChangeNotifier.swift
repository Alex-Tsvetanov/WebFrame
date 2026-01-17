//
//  DocumentChangeNotifier.swift
//  Compiler
//
//  Created by Simon Corsin on 4/7/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

protocol DocumentChangeNotifier {

    func propagateEventData(_ eventData: Data)
}
