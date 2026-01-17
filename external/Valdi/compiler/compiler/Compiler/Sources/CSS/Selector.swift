//
//  Selector.swift
//  Compiler
//
//  Created by Nathaniel Parrott on 5/14/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation
import SwiftCSSParser

class SelectorTerm {
    init() { }
    // represents a selector as a linked list.
    // e.g. in .nav > Label:first-child, 'nav' and `Label:first-child` would be two terms,
    // with the SelectorTerm for `Label:first-child` as the root, with a link to the term for `.nav`.
    // A term without any values set would represent the universal selector
    var idRule: String?
    var classRules: [String] = []
    var tagRule: String?
    var attributeRules: [(attribute: String, value: String, type: Valdi_CSSRuleIndex.AttributeRule.TypeEnum)] = []
    var firstChild = false
    var lastChild = false
    var nthChildRules: [(n: Int?, offset: Int?)] = []
    var parent: (term: SelectorTerm, directChild: Bool)?

    class func parseSelectorToTerms(selector: StyleSelector, termNode: SelectorTerm) throws {
        // returns the last term in the selector, which holds a reference to the term before it, etc
        guard let match = selector.match else { return /* done */ }
        switch match {
        case .className:
            termNode.classRules.append(selector.value!)
        case .id:
            termNode.idRule = selector.value!
        case .pseudoClass:
            guard let psuedoType = selector.psuedoType else {
                throw CompilerError("Unsupported CSS selector: \(selector.value ?? "")")
            }
            switch psuedoType {
            case .firstChild:
                termNode.firstChild = true
            case .lastChild:
                termNode.lastChild = true
            case .nthChild:
                guard let argument = selector.data?.argument else {
                    throw CompilerError("No argument provided for nth-child selector")
                }
                let nth = try parseNthChildArgument(argument)
                termNode.nthChildRules.append(( n: nth.n, offset: nth.offset ))
            default:
                throw CompilerError("Unknown css psuedo-selector: \(psuedoType)")
            }
        case .attributeExact:
            termNode.attributeRules.append((attribute: selector.data!.attribute!, value: selector.value!, type: Valdi_CSSRuleIndex.AttributeRule.TypeEnum.equals))
        case .tag:
            termNode.tagRule = selector.value!
            // TODO: support non-exact attribute selectors, even though they aren't supported by the katana Swift wrapper (need to check the raw c structs)
        }
        if let related = selector.related, let relation = selector.relation, related.match != nil {
            // there's another term before this one, or another part of the current term:
            switch relation {
            case .child, .descendant:
                termNode.parent = (term: SelectorTerm(), directChild: relation == .child)
                try parseSelectorToTerms(selector: related, termNode: termNode.parent!.term)
            case .subselector:
                // additional qualifier on the current term
                try parseSelectorToTerms(selector: related, termNode: termNode)
            default:
                throw CompilerError("Unsupported css relation: \(relation)")
            }
        }
    }

    private class func parseNthChildArgument(_ argument: String) throws -> (n: Int, offset: Int) {
        let argument = argument.trimmed
        if argument == "even" {
            return (n: 2, offset: 0)
        }
        if argument == "odd" {
            return (n: 2, offset: 1)
        }
        if !argument.contains("n") {
            guard let offset = Int(argument) else {
                throw CompilerError("Failed to parse nth-child offset value: '\(argument)'")
            }
            return (n: 0, offset: offset)
        }

        let components = argument.components(separatedBy: "n")
        let nStr = !components.isEmpty ? components[0] : ""
        let offsetStr = components.count > 1 ? components[1].replacingOccurrences(of: " ", with: "") : ""

        guard let n = nStr.isEmpty || nStr == "+" ? 1 : (nStr == "-" ? -1 : Int(nStr)) else {
            throw CompilerError("Failed to parse nth-child n value: '\(nStr)'")
        }
        guard let offset = offsetStr.isEmpty ? 0 : Int(offsetStr) else {
            throw CompilerError("Failed to parse nth-child offset value: '\(offsetStr)'")
        }
        return (n: n, offset: offset)
    }
}
