const wordSplitRegex = /[\W_]/;

export function toPascalCase(str: string): string {
    const words = str.trim().toLowerCase().split(wordSplitRegex);
    return words.reduce((acc, curr) => {
        const [firstChar, ...rest] = curr;
        const pascalCaseWord = firstChar?.toUpperCase()?.concat(rest.join('')) ?? '';
        return acc + pascalCaseWord;
    }, '');
}

export function toSnakeCase(str: string): string {
    const words = str.trim().toLowerCase().split(wordSplitRegex);
    return words.join('_');
}