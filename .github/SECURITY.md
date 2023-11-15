# Security Policy

## Supported Versions

Fixes for security vulnerabilities are applied to any applicable branch/release
series that is in the
[Next-Gen, Active, Maintenance, or Extended support category](https://libjpeg-turbo.org/DeveloperInfo/Versioning).

## Reporting a Vulnerability

Vulnerabilities can be reported in one of the following ways:

- [E-mail the project admin](https://libjpeg-turbo.org/About/Contact).  You can
  optionally encrypt the e-mail using the provided public GPG key.
- Open a
  [GitHub draft security advisory](https://github.com/libjpeg-turbo/libjpeg-turbo/security/advisories/new).
  Note that security advisories are reserved for security researchers who fully
  understand the Common Vulnerability Scoring System (CVSS), Common Weakness
  Enumeration (CWE), and Common Vulnerabilities and Exposures (CVE) Program and
  who are prepared to demonstrate a known or probable exploit for an issue that
  exists in an official release of libjpeg-turbo.  For example, if a buffer
  overrun, an uninitialized read, or undefined behavior can be triggered by
  malformed data passed to a public libjpeg-turbo API function from an
  otherwise well-behaved calling program, then it merits investigation as a
  potential security issue.  If, however, the calling program itself is
  malformed and could not work properly with any image, then its inevitable
  failure is not a security issue.  Such failures can be reported using a
  [GitHub bug report](https://github.com/libjpeg-turbo/libjpeg-turbo/issues/new/choose),
  and they will be investigated as potential opportunities for user proofing.
- [Beta and Post-Beta release series](https://libjpeg-turbo.org/DeveloperInfo/Versioning)
  are not expected to be free of bugs, so vulnerabilities that affect only
  those release series (for example, vulnerabilities introduced by a new
  feature that is not present in a Stable release series) can optionally be
  reported using a
  [GitHub bug report](https://github.com/libjpeg-turbo/libjpeg-turbo/issues/new/choose).
  Vulnerabilities that affect only
  [Alpha/Evolving release series](https://libjpeg-turbo.org/DeveloperInfo/Versioning)
  should always be reported using a
  [GitHub bug report](https://github.com/libjpeg-turbo/libjpeg-turbo/issues/new/choose).
