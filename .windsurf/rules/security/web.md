---
trigger: glob
globs: **/*.dart,**/*.js,**/*.ts,**/*.html
description: Web-specific security rules for browser and web server targets
---

# Web Security Rules

## 1. Secure Storage in the Browser
- **Never store sensitive data in `localStorage` or `sessionStorage`** — these are accessible to any JavaScript on the page (XSS risk).
- **Prefer HTTP-only, Secure, SameSite=Strict cookies** for session tokens. HTTP-only cookies cannot be read by JavaScript.
- **Encrypted IndexedDB** is acceptable for offline data that must persist, but the encryption key must never be stored in plaintext in the same origin.

## 2. Content Security Policy (CSP)
- **Every response must include a `Content-Security-Policy` header.** Default policy must deny inline scripts (`script-src 'self'`) and restrict resource origins to known trusted sources.
- **Never use `unsafe-inline` or `unsafe-eval`** in CSP for production builds. These negate the protection CSP provides.
- **Subresource Integrity (SRI):** All third-party scripts and stylesheets loaded from a CDN must include `integrity` and `crossorigin` attributes.

## 3. HTTPS & Transport Security
- **Enforce HTTPS everywhere.** Redirect all HTTP requests to HTTPS at the server layer — never serve sensitive content over plain HTTP.
- **HSTS:** Include `Strict-Transport-Security: max-age=31536000; includeSubDomains` in all HTTPS responses.
- **Secure and SameSite cookie attributes** must be set on all cookies: `Secure; HttpOnly; SameSite=Strict` (or `Lax` where cross-site GET is intentional).

## 4. Cross-Origin & CORS
- **Do not use `Access-Control-Allow-Origin: *`** for endpoints that handle authentication or sensitive data. Always scope allowed origins to known trusted domains.
- **CORS preflight responses** must not cache credentials (`Access-Control-Allow-Credentials: true`) alongside a wildcard origin — this is a security misconfiguration.

## 5. XSS & Injection Prevention
- **Never inject user-supplied content directly into HTML** templates (Inja or otherwise) without escaping. Always use the template engine's auto-escaping features.
- **Validate and sanitize** all URL parameters, query strings, and form inputs server-side before processing or storing.
- **JSON responses** must set `Content-Type: application/json` explicitly to prevent MIME sniffing attacks.
