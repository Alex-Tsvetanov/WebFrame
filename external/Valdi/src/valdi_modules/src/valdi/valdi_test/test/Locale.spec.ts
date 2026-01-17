import { Locale } from 'valdi_core/src/localization/Locale';
import 'jasmine/src/jasmine';

describe('Locale', () => {
  it('can parse with language and region', () => {
    const locale = Locale.parse('fr-FR');

    expect(locale.language).toBe('fr');
    expect(locale.region).toBe('FR');
  });

  it('can parse without region', () => {
    const locale = Locale.parse('fr');

    expect(locale.language).toBe('fr');
    expect(locale.region).toBeUndefined();
  });

  it('uses lowercase language and uppercase region', () => {
    const locale = Locale.parse('FR-fr');

    expect(locale.language).toBe('fr');
    expect(locale.region).toBe('FR');
  });
});
