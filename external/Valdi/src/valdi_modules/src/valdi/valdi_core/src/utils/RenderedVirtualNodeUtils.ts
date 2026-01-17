import { IRenderedVirtualNode } from 'valdi_core/src/IRenderedVirtualNode';

function outputXMLAttributeValue(value: any): string {
  if (value === undefined) {
    return '<undefined>';
  } else if (value === null) {
    return '<null>';
  } else if (typeof value === 'function') {
    return '<function>';
  } else if (Array.isArray(value)) {
    const values = value.map(outputXMLAttributeValue).join(', ');
    return `[${values}]`;
  } else if (value.toString) {
    return value.toString();
  } else {
    return '<unknown>';
  }
}

function outputXML(virtualNode: IRenderedVirtualNode, indent: string, output: string[], recursive: boolean) {
  output.push(indent);
  output.push('<');

  const xmlTag = getNodeTag(virtualNode);
  output.push(xmlTag);

  if (virtualNode.element) {
    const element = virtualNode.element;

    for (const attributeName of element.getAttributeNames()) {
      const attributeValue = outputXMLAttributeValue(element.getAttribute(attributeName));

      output.push(' ');
      output.push(attributeName);
      output.push('="');
      output.push(attributeValue.toString());
      output.push('"');
    }
  }

  if (virtualNode.children.length && recursive) {
    output.push('>\n');
    const childIndent = indent + '  ';

    for (const child of virtualNode.children) {
      outputXML(child, childIndent, output, recursive);
    }

    output.push(indent);
    output.push('</');
    output.push(xmlTag);
    output.push('>\n');
  } else {
    output.push('/>\n');
  }
}

export function getNodeTag(virtualNode: IRenderedVirtualNode): string {
  if (virtualNode.element) {
    return virtualNode.element.tag;
  } else if (virtualNode.component) {
    return (virtualNode.component as any).constructor.name;
  }
  return 'undefined';
}

export function toXML(virtualNode: IRenderedVirtualNode, recursive: boolean): string {
  const allStrings: string[] = [];
  outputXML(virtualNode, '', allStrings, recursive);

  return allStrings.join('');
}

export function computeUniqueId(virtualNode: IRenderedVirtualNode): string {
  const parts: (string | number)[] = [];
  let current: IRenderedVirtualNode | undefined = virtualNode;
  while (current !== undefined) {
    const element = current.element;
    if (element !== undefined) {
      parts.push(element.id);
      break;
    }
    parts.push(current.key);
    current = current.parent;
  }
  return parts.join('/');
}
