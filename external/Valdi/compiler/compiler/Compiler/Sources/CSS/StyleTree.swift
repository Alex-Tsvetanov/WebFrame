//
//  StyleTree.swift
//  Compiler
//
//  Created by Nathaniel Parrott on 5/14/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation
import SwiftCSSParser

class StyleTree {

    init() {

    }

    let root = StyleNode()

    func add(logger: ILogger, cssString: String, filename: String, onImport: ((String) throws -> Void)?) throws {
        if cssString.isEmpty {
            return
        }

        guard let parsed = StyleSheet(string: cssString) else {
            throw CompilerError("Failed to parse CSS from \(filename)")
        }
        if let cssErr = parsed.errors.first {
            throw cssErr
        }
        for importFilename in parsed.importFilenames {
            try onImport?(importFilename)
        }
        for rule in parsed.rules {
            for selector in rule.selectors {
                let rootSelectorTerm = SelectorTerm()
                try SelectorTerm.parseSelectorToTerms(selector: selector, termNode: rootSelectorTerm)
                root.add(logger: logger, declarations: rule.declarations, forSelectorTerm: rootSelectorTerm, specificity: selector.specificity!, totalRuleCount: totalRuleCount)
            }
        }
    }

    class TotalRuleCount {
        init() { }
        var count = 0
    }
    let totalRuleCount = TotalRuleCount()
}

class StyleNode {
    init() { }

    var declarations: [Valdi_StyleDeclaration] = []
    var idRules: [String: StyleNode] = [:]
    var classRules: [String: StyleNode] = [:]
    var tagRules: [String: StyleNode] = [:]
    var attribRules: [AttribRule] = []
    var firstChildRule: StyleNode?
    var lastChildRule: StyleNode?
    var nthChildRules: [NthChildRule] = []
    var ancestorRule: StyleNode?
    var directParentRule: StyleNode?

    struct AttribRule {
        let attrib: String
        let value: String
        let type: Valdi_CSSRuleIndex.AttributeRule.TypeEnum
        let node: StyleNode
    }

    struct NthChildRule {
        let n: Int?
        let offset: Int?
        let node: StyleNode
    }

    func add(logger: ILogger, declarations: [StyleDeclaration], forSelectorTerm term: SelectorTerm, specificity: Int, totalRuleCount: StyleTree.TotalRuleCount) {
        // When building (and querying) the tree, it's important
        // to be consistent in our ordering of selector rules.
        // Label.classA.classB is semantically identical to .classB.classA.Label,
        // so they need to be added to the tree in a consistent order.
        // [.classA] -> [.classB] -> [Label]
        // This means sorting classes and attribute selectors, and adding
        // rules in the order they're specified in the proto -- id -> class -> tag -> attrib -> firstChild -> lastChild -> nthChild.
        var curNode = self
        if let idString = term.idRule {
            if curNode.idRules[idString] == nil {
                curNode.idRules[idString] = StyleNode()
            }
            curNode = curNode.idRules[idString]!
        }
        for classString in term.classRules.sorted() {
            if curNode.classRules[classString] == nil {
                curNode.classRules[classString] = StyleNode()
            }
            curNode = curNode.classRules[classString]!
        }
        if let tag = term.tagRule {
            if curNode.tagRules[tag] == nil {
                curNode.tagRules[tag] = StyleNode()
            }
            curNode = curNode.tagRules[tag]!
        }
        for (attribute, value, type) in term.attributeRules.sorted(by: { (attr1, attr2) -> Bool in
            return attr1.attribute <= attr2.attribute
        }) {
            if let existingNode = curNode.attribRules.filter({ $0.attrib == attribute && $0.value == value && $0.type == type }).first?.node {
                curNode = existingNode
            } else {
                let newNode = StyleNode()
                curNode.attribRules.append(AttribRule(attrib: attribute.camelCased, value: value, type: type, node: newNode))
                curNode = newNode
            }
        }
        if term.firstChild {
            if curNode.firstChildRule == nil {
                curNode.firstChildRule = StyleNode()
            }
            curNode = curNode.firstChildRule!
        }
        if term.lastChild {
            if curNode.lastChildRule == nil {
                curNode.lastChildRule = StyleNode()
            }
            curNode = curNode.lastChildRule!
        }
        for (n, offset) in term.nthChildRules.sorted(by: { (item1, item2) -> Bool in
            if item1.n ?? 0 < item2.n ?? 0 {
                return true
            }
            return item1.offset ?? 0 <= item2.offset ?? 0
        }) {
            if let existingNode = curNode.nthChildRules.filter({ $0.n == n && $0.offset == offset }).first?.node {
                curNode = existingNode
            } else {
                let newNode = StyleNode()
                curNode.nthChildRules.append(NthChildRule(n: n, offset: offset, node: newNode))
                curNode = newNode
            }
        }
        if let (term, directChild) = term.parent {
            if directChild {
                if curNode.directParentRule == nil {
                    curNode.directParentRule = StyleNode()
                }
                curNode.directParentRule!.add(logger: logger, declarations: declarations, forSelectorTerm: term, specificity: specificity, totalRuleCount: totalRuleCount)
            } else {
                if curNode.ancestorRule == nil {
                    curNode.ancestorRule = StyleNode()
                }
                curNode.ancestorRule!.add(logger: logger, declarations: declarations, forSelectorTerm: term, specificity: specificity, totalRuleCount: totalRuleCount)
            }
        } else {
            // add declarations:
            for declaration in declarations {
                var declarationProto = Valdi_StyleDeclaration()
                declarationProto.priority = Int32(specificity)
                declarationProto.order = Int32(totalRuleCount.count)
                declarationProto.attribute = Valdi_NodeAttribute(logger: logger, name: declaration.name.camelCased, value: declaration.value)
                declarationProto.id = declarationProto.order // order is unique to each compile, so we can use it as our ID
                totalRuleCount.count += 1
                curNode.declarations.append(declarationProto)
            }
        }
    }
}
