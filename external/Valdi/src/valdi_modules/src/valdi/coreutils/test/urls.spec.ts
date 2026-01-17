import * as urls from 'coreutils/src/url';
import 'jasmine/src/jasmine';

describe('coreutils > url', () => {
  it('should encode query params', () => {
    expect(urls.encodeQueryParams({})).toEqual('');
    expect(urls.encodeQueryParams({ foo: 'bar' })).toEqual('foo=bar');
    expect(urls.encodeQueryParams({ foo: 'bar bar' })).toEqual('foo=bar%20bar');
    expect(urls.encodeQueryParams({ foo: 'bar', bar: 'baz' })).toEqual('foo=bar&bar=baz');
    expect(urls.encodeQueryParams({ foo: undefined })).toEqual('');
    expect(urls.encodeQueryParams({ foo: 200 })).toEqual('foo=200');
    expect(urls.encodeQueryParams({ 'foo bar': 'baz' })).toEqual('foo%20bar=baz');
  });

  it('should decode query params', () => {
    expect(urls.decodeQueryParams('')).toEqual({});
    expect(urls.decodeQueryParams('foo=bar')).toEqual({ foo: 'bar' });
    expect(urls.decodeQueryParams('foo=bar%20bar')).toEqual({ foo: 'bar bar' });
    expect(urls.decodeQueryParams('foo=bar&bar=baz')).toEqual({ foo: 'bar', bar: 'baz' });
    expect(urls.decodeQueryParams('foo=200')).toEqual({ foo: '200' });
    expect(urls.decodeQueryParams('foo=bar&foo=baz')).toEqual({ foo: 'baz' });
  });

  it('should create a url with params', () => {
    expect(urls.pathWithParams('', { foo: 'bar', bar: 'baz' })).toEqual('?foo=bar&bar=baz');
    expect(urls.pathWithParams('http://foo', { foo: 'bar', bar: 'baz' })).toEqual('http://foo?foo=bar&bar=baz');
    expect(urls.pathWithParams('http://foo', {})).toEqual('http://foo?');
  });

  it('should return params from a url', () => {
    expect(urls.paramsFromUrl('')).toEqual({});
    expect(urls.paramsFromUrl('http://foo')).toEqual({});
    expect(urls.paramsFromUrl('http://foo?')).toEqual({});
    expect(urls.paramsFromUrl('http://foo?foo=bar')).toEqual({ foo: 'bar' });
    expect(urls.paramsFromUrl('http://foo?foo=bar%20bar&bar=baz')).toEqual({ foo: 'bar bar', bar: 'baz' });
  });

  it('should return hostname from a url', () => {
    expect(urls.hostnameFromUrl('')).toEqual('');
    expect(urls.hostnameFromUrl('foo')).toEqual('');
    expect(urls.hostnameFromUrl('https://foo')).toEqual('foo');
    expect(urls.hostnameFromUrl('https://foo?')).toEqual('foo');
    expect(urls.hostnameFromUrl('https://foo?foo=bar')).toEqual('foo');
    expect(urls.hostnameFromUrl('https://foo?foo=bar%20bar&bar=baz')).toEqual('foo');
    expect(urls.hostnameFromUrl('https://foo.foo')).toEqual('foo.foo');
    expect(urls.hostnameFromUrl('http://foo')).toEqual('foo');
  });
});
