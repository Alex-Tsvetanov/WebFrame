//
//  Stylesheet.swift
//  TemplateKit
//
//  Created by Matias Cudich on 9/27/16.
//  Copyright © 2016 Matias Cudich. All rights reserved.
//

import Foundation
import KatanaParser

public enum Match {
  case tag
  case id
  case className
  case attributeExact
  case pseudoClass

  init(_ rawValue: KatanaSelectorMatch) {
    switch rawValue {
    case KatanaSelectorMatchTag:
      self = .tag
    case KatanaSelectorMatchId:
      self = .id
    case KatanaSelectorMatchClass:
      self = .className
    case KatanaSelectorMatchAttributeExact:
      self = .attributeExact
    case KatanaSelectorMatchPseudoClass:
      self = .pseudoClass
    default:
      fatalError()
    }
  }
}

public enum Relation {
  case subselector
  case descendant
  case child
  case directAdjacent
  case indirectAdjacent

  init(_ rawValue: KatanaSelectorRelation) {
    switch rawValue {
    case KatanaSelectorRelationSubSelector:
      self = .subselector
    case KatanaSelectorRelationDescendant:
      self = .descendant
    case KatanaSelectorRelationChild:
      self = .child
    case KatanaSelectorRelationDirectAdjacent:
      self = .directAdjacent
    case KatanaSelectorRelationIndirectAdjacent:
      self = .indirectAdjacent
    default:
      fatalError()
    }
  }
}

public struct Rule {
  public let selectors: [StyleSelector]
  public let declarations: [StyleDeclaration]

  public init(selectors: [StyleSelector], declarations: [StyleDeclaration]) {
    self.selectors = selectors
    self.declarations = declarations
  }

  public func matches(_ element: StyleElement) -> Bool {
    return selectors.contains { selector in
      return selector.matches(element)
    }
  }

  public func greatestSpecificity(for element: StyleElement) -> Int {
    var specificity = 0
    for selector in selectors {
      if selector.matches(element) {
        var underlyingSelector = selector.selector!
        specificity = max(specificity, Int(katana_calc_specificity_for_selector(&underlyingSelector)))
      }
    }
    return specificity
  }
}

public struct RareData {
  public var value: String?
  public var attribute: String?
  public var argument: String?
}

public enum PseudoType {
  case empty
  case firstChild
  case firstOfType
  case lastChild
  case lastOfType
  case onlyChild
  case onlyOfType
  case nthChild
  case nthOfType
  case nthLastChild
  case nthLastOfType
  case pseudoFocus
  case pseudoActive
  case pseudoEnabled
  case pseudoDisabled

  public init?(_ rawValue: KatanaPseudoType) {
    switch rawValue {
    case KatanaPseudoEmpty:
      self = .empty
    case KatanaPseudoFirstChild:
      self = .firstChild
    case KatanaPseudoFirstOfType:
      self = .firstOfType
    case KatanaPseudoLastChild:
      self = .lastChild
    case KatanaPseudoLastOfType:
      self = .lastOfType
    case KatanaPseudoOnlyChild:
      self = .onlyChild
    case KatanaPseudoOnlyOfType:
      self = .onlyOfType
    case KatanaPseudoNthChild:
      self = .nthChild
    case KatanaPseudoNthOfType:
      self = .nthOfType
    case KatanaPseudoNthLastChild:
      self = .nthLastChild
    case KatanaPseudoNthLastOfType:
      self = .nthLastOfType
    case KatanaPseudoFocus:
      self = .pseudoFocus
    case KatanaPseudoActive:
      self = .pseudoActive
    case KatanaPseudoEnabled:
      self = .pseudoEnabled
    case KatanaPseudoDisabled:
      self = .pseudoDisabled
    default:
      return nil
    }
  }
}

public struct StyleSelectorData {
  var match: Match?
  var relation: Relation?
  var selector: KatanaSelector?
  var data: RareData?
  var pseudoType: PseudoType?
  var value: String?
}

public indirect enum StyleSelector {
  case none
  case some(StyleSelectorData, StyleSelector)

  public var match: Match? {
    if case let .some(data, _) = self {
      return data.match
    }
    return nil
  }

  public var related: StyleSelector? {
    if case let .some(_, related) = self {
      return related
    }
    return nil
  }

  public var relation: Relation? {
    if case let .some(data, _) = self {
      return data.relation
    }
    return nil
  }

  public var selector: KatanaSelector? {
    if case let .some(data, _) = self {
      return data.selector
    }
    return nil
  }

  public var psuedoType: PseudoType? {
    if case let .some(data, _) = self {
      return data.pseudoType
    }
    return nil
  }

  public var data: RareData? {
    if case let .some(data, _) = self {
      return data.data
    }
    return nil
  }

  public var value: String? {
    if case let .some(data, _) = self {
      return data.value
    }
    return nil
  }

    public var specificity: Int? {
        if case let .some(data, _) = self {
            var underlyingSelector = data.selector!
            return Int(katana_calc_specificity_for_selector(&underlyingSelector))
        }
        return nil
    }

  public func matches(_ element: StyleElement) -> Bool {
    if case .none = self {
      return true
    }

    guard let match = match, let value = value else {
      return false
    }

    var matches: Bool
    switch match {
    case .id:
      matches = element.id == value
    case .className:
      matches = element.classNames?.contains(value) ?? false
    case .tag:
      matches = element.tagName == value
    case .attributeExact:
      if let data = data, let attribute = data.attribute, let value = data.value {
        matches = element.has(attribute: attribute, with: value)
      } else {
        matches = false
      }
    case .pseudoClass:
      guard let psuedoType = psuedoType else {
        matches = false
        break
      }
      switch psuedoType {
      case .firstChild:
        let precedingSiblings = element.parentElement?.indirectAdjacents(of: element) ?? []
        matches = precedingSiblings.count == 0
      case .firstOfType:
        let precedingSiblings = element.parentElement?.indirectAdjacents(of: element) ?? []
        matches = !precedingSiblings.contains { sibling in
          return sibling.tagName == element.tagName
        }
      case .lastChild:
        let precedingSiblings = element.parentElement?.indirectAdjacents(of: element) ?? []
        matches = precedingSiblings.count == (element.parentElement?.childElements?.count ?? 0) - 1
      case .lastOfType:
        let subsequentSiblings = element.parentElement?.subsequentAdjacents(of: element) ?? []
        matches = !subsequentSiblings.contains { sibling in
          sibling.tagName == element.tagName
        }
      case .onlyChild:
        matches = element.parentElement?.childElements?.count == 1
      case .onlyOfType:
        let precedingSiblings = element.parentElement?.indirectAdjacents(of: element) ?? []
        let subsequentSiblings = element.parentElement?.subsequentAdjacents(of: element) ?? []
        matches = !(precedingSiblings + subsequentSiblings).contains { sibling in
          return sibling.tagName == element.tagName
        }
      case .pseudoFocus:
        matches = element.isFocused
      case .pseudoEnabled:
        matches = element.isEnabled
      case .pseudoDisabled:
        matches = !element.isEnabled
      case .pseudoActive:
        matches = element.isActive
      default:
        matches = false
      }
    }

    if !matches {
      return false
    }

    guard let relation = relation, let related = related else {
      return true
    }

    switch relation {
    case .subselector:
      return related.matches(element)
    case .descendant:
      var parent = element.parentElement
      while let currentParent = parent {
        if related.matches(currentParent) {
          return true
        }
        parent = currentParent.parentElement
      }
      return false
    case .child:
      guard let parent = element.parentElement else {
        return false
      }
      return related.matches(parent)
    case .directAdjacent:
      guard let directAdjacent = element.parentElement?.directAdjacent(of: element) else {
        return false
      }
      return related.matches(directAdjacent)
    case .indirectAdjacent:
      guard let indirectAdjacents = element.parentElement?.indirectAdjacents(of: element) else {
        return false
      }
      return indirectAdjacents.contains { indirectAdjacent in
        return related.matches(indirectAdjacent)
      }
    }
  }
}

public struct StyleDeclaration {
  public let name: String
  public let value: String
  public let important: Bool
}

public struct CSSError: Error {
  let message: String
  let atZeroIndexedLine: Int
  let column: Int
}

public struct StyleSheet: Equatable {
  public var rules = [Rule]()
  public var importFilenames = [String]()
  public private(set) var errors = [CSSError]()

  fileprivate let data: Data?
  private let inheritedProperties: Set<String>

  public init() {
    data = nil
    inheritedProperties = Set([])
  }

  public init?(string: String, inheritedProperties: [String]? = nil) {
    guard !string.isEmpty, let data = string.data(using: String.Encoding.utf8) else {
      return nil
    }
    self.data = data
    self.inheritedProperties = Set(inheritedProperties ?? [])

    data.withUnsafeBytes { unsafeRawBufferPointer in
      let unsafeBufferPointer = unsafeRawBufferPointer.bindMemory(to: Int8.self)
      guard let bytes = unsafeBufferPointer.baseAddress else {
        return
      }
      guard let parsed = katana_parse(bytes, data.count, KatanaParserModeStylesheet) else {
        return
      }
      let stylesheet = parsed.pointee.stylesheet.pointee

      let errors: [KatanaError] = fromKatanaArray(array: parsed.pointee.errors)
      self.errors = errors.map() { err -> CSSError in
        return CSSError(message: "invalid syntax", atZeroIndexedLine: Int(err.last_line), column: Int(err.last_column))
      }

      let importStatements: [KatanaImportRule] = fromKatanaArray(array: stylesheet.imports)
      self.importFilenames = importStatements.map() { String(cString: $0.href) }

      let rules: [KatanaStyleRule] = fromKatanaArray(array: stylesheet.rules)
      self.rules = rules.map { rule in
        let parsedSelectors: [KatanaSelector] = fromKatanaArray(array: rule.selectors.pointee)
        let selectors = parsedSelectors.map { selector in
          return makeSelector(selector)
        }
        let parsedDeclarations: [KatanaDeclaration] = fromKatanaArray(array: rule.declarations.pointee)
        let declarations = parsedDeclarations.map { declaration in
          return makeDeclaration(declaration)
        }
        return Rule(selectors: selectors, declarations: declarations)
      }
    }
  }

  public func rulesForElement(_ element: StyleElement) -> [Rule] {
    return rules.filter { rule in
      return rule.matches(element)
    }
  }

  public func stylesForElement(_ element: StyleElement) -> [String: StyleDeclaration] {
    var declarations = [String: (Rule, StyleDeclaration)]()
    for rule in rulesForElement(element) {
      for declaration in rule.declarations {
        if let (existingRule, _) = declarations[declaration.name] {
          let existingSpecificity = existingRule.greatestSpecificity(for: element)
          let newSpecificity = rule.greatestSpecificity(for: element)
          if newSpecificity >= existingSpecificity {
            declarations[declaration.name] = (rule, declaration)
          }
        } else {
          declarations[declaration.name] = (rule, declaration)
        }
      }
    }

    var declarationMap = [String: StyleDeclaration]()
    var keys = Set<String>()
    for (key, (_, declaration)) in declarations {
      declarationMap[key] = declaration
      keys.insert(key)
    }
    let remainingInheritedProperties = inheritedProperties.subtracting(keys)

    if let parent = element.parentElement, remainingInheritedProperties.count > 0 {
      let parentStyles = stylesForElement(parent)
      for (property, declaration) in parentStyles {
        if remainingInheritedProperties.contains(property) {
          declarationMap[property] = declaration
        }
      }
    }

    return declarationMap
  }

  public func apply(to element: StyleElement) {
    let matchingStyles = stylesForElement(element)
    var styleSheetProperties = [String: Any]()
    for (name, declaration) in matchingStyles {
      styleSheetProperties[name] = declaration.value
    }

    element.apply(styles: styleSheetProperties)

    for child in element.childElements ?? [] {
      apply(to: child)
    }
  }

  private func makeSelector(_ selector: KatanaSelector) -> StyleSelector {
    let parent = selector.tagHistory == nil ? .none : makeSelector(selector.tagHistory.pointee)

    var value = ""
    if selector.tag != nil {
      value = String(cString: selector.tag.pointee.local)
    } else {
      value = String(cString: selector.data.pointee.value)
    }

    var rareData = RareData()
    if selector.data.pointee.value != nil {
      rareData.value = String(cString: selector.data.pointee.value)
    }
    if selector.data.pointee.attribute != nil {
      rareData.attribute = String(cString: selector.data.pointee.attribute.pointee.local)
    }
    if selector.data.pointee.argument != nil {
        rareData.argument = String(cString: selector.data.pointee.argument)
    }

    let data = StyleSelectorData(match: Match(selector.match), relation: Relation(selector.relation), selector: selector, data: rareData, pseudoType: PseudoType(selector.pseudo), value: value)

    return .some(data, parent)
  }

  private func makeDeclaration(_ declaration: KatanaDeclaration) -> StyleDeclaration {
    return StyleDeclaration(name: String(cString: declaration.property), value: String(cString: declaration.raw), important: declaration.important)
    // return StyleDeclaration(name: String(cString: declaration.property), value: makeValue(declaration.values.pointee), important: declaration.important)
  }

  private func makeValue(_ values: KatanaArray) -> String {
    let values: [KatanaValue] = fromKatanaArray(array: values)
    return values.map { value in
      if value.isInt {
        return String(cString: value.raw) + stringForUnit(value.unit)
      } else if value.fValue >= Double.leastNormalMagnitude {
        return "\(value.fValue)" + stringForUnit(value.unit)
      } else {
        return String(cString: value.string)
      }
    }.joined(separator: " ")
  }

  private func fromKatanaArray<T>(array: KatanaArray) -> [T] {
    var data = array.data
    var results = [T]()
    for _ in 0..<array.length {
      guard let rule = data?.pointee?.assumingMemoryBound(to: T.self).pointee else {
        continue
      }
      data = data?.advanced(by: 1)
      results.append(rule)
    }
    return results
  }
}

public func ==(lhs: StyleSheet, rhs: StyleSheet) -> Bool {
  return lhs.data == rhs.data
}

private func stringForUnit(_ unit: KatanaValueUnit) -> String {
    switch unit {
    case KATANA_VALUE_PX:
        return "px"
    case KATANA_VALUE_PT:
        return "pt"
    case KATANA_VALUE_VW:
        return "vw"
    case KATANA_VALUE_VH:
        return "vh"
    case KATANA_VALUE_PERCENTAGE:
        return "%"
    default:
        return ""
    }
}
