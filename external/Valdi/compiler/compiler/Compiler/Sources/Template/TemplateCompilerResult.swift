//
//  TemplateCompilerResult.swift
//  Compiler
//
//  Created by Simon Corsin on 4/7/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

struct ObservedElement {
    let elementId: String
    let attributeValue: ValdiRawNodeAttribute
}

struct TemplateCompilerResult {

    let rootElement: ValdiJSElement
    var actions: [ValdiAction]

}
