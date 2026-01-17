import * as path from 'path';
import axios from 'axios';
import * as fs from 'fs';

const PROJECT_ROOT_PATH = path.resolve(path.join(__dirname, '../../../..'));
const EMOJI_SEQUENCES_URL = 'https://unicode.org/Public/emoji/latest/emoji-sequences.txt';
const OUTPUT_FILE_PATH_RELATIVE = 'src/valdi/runtime/Text/Emoji_Gen.hpp';
const OUTPUT_FILE_PATH_ABSOLUTE = path.join(PROJECT_ROOT_PATH, OUTPUT_FILE_PATH_RELATIVE);

interface Emoji {
  readonly codepoints: number[];
}

interface EmojiRange {
  start: number;
  length: number;
}

/**
 * Unicode is 21 bits, we use 3 bits for identifying the opcode,
 * 8 bits to count the number of number of entries.
 *
 * Opcodes:
 *
 * Single Codepoint (32bits entry):
 * opcode 0x1 3bits
 * entries num 8bits
 * starting unicode point 21bits
 *
 * Emoji Sequence (32bits + (n * 32bits))
 * opcode 0x2 3bits
 * entries num 8bits
 * modifier unicode point 21bits
 * ... [entries num 8 bits + unicode points 21 bits] (entries num * 32 bits)
 *
 * Complex Emoji Sequence (32bits + (n * 32 bits))
 * opcode 0x3 bits
 * sequence length 8bits
 * first unicode points 21 bits
 * ... unicode points ((sequence length - 1) * 32 bits)
 */

const SINGLE_CODEPOINT_OPCODE = 0x1;
const EMOJI_SEQUENCE_OPCODE = 0x2;
const COMPLEX_EMOJI_SEQUENCE_OPCODE = 0x3;
const MAX_UNICODE_VALUE = ~0 >>> 11;

function makeCompressedTableEntry(opcode: number, sequenceLength: number, unicodePoint: number): number {
  if (sequenceLength > 0xff) {
    throw new Error(`Sequence length cannot go beyond 0xFF (requested ${sequenceLength})`);
  }
  if (unicodePoint > MAX_UNICODE_VALUE) {
    throw new Error(`Unicode point ${unicodePoint} is beyond the maximum ${MAX_UNICODE_VALUE}`);
  }

  return (opcode << 29) | (sequenceLength << 21) | unicodePoint;
}

function parseCodepoint(codepointStr: string): number {
  return parseInt(codepointStr, 16);
}

function computeEmojiRanges(codepoints: number[]): EmojiRange[] {
  codepoints.sort((a, b) => a - b);

  const emojiRanges: EmojiRange[] = [];
  let currentRange: EmojiRange | undefined;
  for (const singleCodepoint of codepoints) {
    if (!currentRange || currentRange.start + currentRange.length != singleCodepoint) {
      currentRange = { start: singleCodepoint, length: 1 };
      emojiRanges.push(currentRange);
    } else {
      currentRange.length++;
    }
  }

  return emojiRanges;
}

function generateCompressedTable(emojis: Emoji[]): string {
  const singleCodepoints: number[] = [];
  const doubleCodepoints: [number, number][] = [];
  const multipleCodepoints: number[][] = [];

  for (const emoji of emojis) {
    if (emoji.codepoints.length === 1) {
      singleCodepoints.push(emoji.codepoints[0]);
    } else if (emoji.codepoints.length === 2) {
      doubleCodepoints.push(emoji.codepoints as [number, number]);
    } else {
      multipleCodepoints.push(emoji.codepoints);
    }
  }

  const singleEmojiRanges = computeEmojiRanges(singleCodepoints);

  const codepointsByEmojiSequenceModifier = new Map<number, number[]>();
  for (const doubleCodepoint of doubleCodepoints) {
    let entries = codepointsByEmojiSequenceModifier.get(doubleCodepoint[1]);
    if (!entries) {
      entries = [];
      codepointsByEmojiSequenceModifier.set(doubleCodepoint[1], entries);
    }
    entries.push(doubleCodepoint[0]);
  }

  const keys = [...codepointsByEmojiSequenceModifier.keys()].sort((a, b) => a - b);
  const compressedTable: number[] = [];

  for (const singleEmojiRange of singleEmojiRanges) {
    compressedTable.push(
      makeCompressedTableEntry(SINGLE_CODEPOINT_OPCODE, singleEmojiRange.length, singleEmojiRange.start),
    );
  }

  for (const emojiModifier of keys) {
    const codepoints = codepointsByEmojiSequenceModifier.get(emojiModifier)!;
    const emojiRanges = computeEmojiRanges(codepoints);
    compressedTable.push(makeCompressedTableEntry(EMOJI_SEQUENCE_OPCODE, emojiRanges.length, emojiModifier));
    for (const emojiRange of emojiRanges) {
      compressedTable.push(makeCompressedTableEntry(0, emojiRange.length, emojiRange.start));
    }
  }

  for (const multipleCodepoint of multipleCodepoints) {
    compressedTable.push(
      makeCompressedTableEntry(COMPLEX_EMOJI_SEQUENCE_OPCODE, multipleCodepoint.length, multipleCodepoint[0]),
    );
    for (let i = 1; i < multipleCodepoint.length; i++) {
      compressedTable.push(multipleCodepoint[i]);
    }
  }

  console.log(`Compressed table length: ${compressedTable.length} (${compressedTable.length * 4} bytes)`);

  const serializedCompressedTable = compressedTable
    .map(n => n.toString(16))
    .map(c => `0x${c}`)
    .join(', ');

  let output = `
  #include <array>
  #include <cstdint>

  // GENERATED EMOJI COMPRESSED TABLE. DO NOT EDIT.
  constexpr std::array<uint32_t, ${compressedTable.length}> kVALDI_EMOJI_COMPRESSED_TABLE = { ${serializedCompressedTable} };

  `;

  return output;
}

function parseEmojiSequenceFile(str: string): Emoji[] {
  const output: Emoji[] = [];

  const entries = str
    .split('\n')
    .map(l => l.trim())
    .filter(l => !l.startsWith('#') && l.length > 0);

  for (const entry of entries) {
    const component = entry.split(';')[0].trim();
    if (component.indexOf('..') >= 0) {
      const components = component.split('..');
      const rangeStart = parseCodepoint(components[0]);
      const rangeEnd = parseCodepoint(components[1]);

      for (let codepoint = rangeStart; codepoint <= rangeEnd; codepoint++) {
        output.push({
          codepoints: [codepoint],
        });
      }
    } else if (component.indexOf(' ') >= 0) {
      const components = component.split(' ');
      const codepoints = components.map(parseCodepoint);
      output.push({
        codepoints,
      });
    } else {
      const codepoint = parseCodepoint(component);
      output.push({
        codepoints: [codepoint],
      });
    }
  }

  console.log(`Number of emojis ${output.length}`);
  console.log(`Number of Emoji with 1 codepoint ${output.filter(f => f.codepoints.length === 1).length}`);

  let lowestValue = Number.MAX_SAFE_INTEGER;
  let highestValue = 0;
  let numberOfCodepoints = 0;
  const modifierSequences = new Set<number>();
  for (const emoji of output) {
    for (const codepoint of emoji.codepoints) {
      lowestValue = Math.min(codepoint, lowestValue);
      highestValue = Math.max(codepoint, highestValue);
      numberOfCodepoints++;
    }

    if (emoji.codepoints.length === 2) {
      modifierSequences.add(emoji.codepoints[1]);
    }
  }

  console.log('Highest value', highestValue);
  console.log('Lowest value', lowestValue);
  console.log('Number of codepoints', numberOfCodepoints);
  console.log('Number of modifier sequences', modifierSequences.size);

  return output;
}

async function fetchEmojiSequenceFile(): Promise<string> {
  console.log('Fetching Emoji sequences file');
  const response = await axios.get(EMOJI_SEQUENCES_URL);
  return response.data;
}

async function main(): Promise<void> {
  const fileContent = await fetchEmojiSequenceFile();
  const sequences = parseEmojiSequenceFile(fileContent);
  const compressedTable = generateCompressedTable(sequences);

  fs.writeFileSync(OUTPUT_FILE_PATH_ABSOLUTE, compressedTable);

  console.log(`Wrote compressed table to ${OUTPUT_FILE_PATH_ABSOLUTE}`);
}

main();
