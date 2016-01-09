# Change Log
All notable changes to this project will be documented in this file.

## [Unreleased]

## [7] - 2016-01-09
### Fixed
- Do not use functions deprecated starting with `libnotmuch` 4.3; building
  against older versions is still possible.

## [6] - 2015-11-06
### Added
- This change log file.

### Fixed
- Fix sorting of results. (Patch by Jeremy Simon
  <[jtsymon@gmail.com](mailto:jtsymon@gmail.com)>.)

## [5] - 2015-06-05
### Fixed
- Normal output was missing angle brackets around e-mail addresses. This issue
  is fixed in this release.

## [4] - 2015-05-21
### Fixed
- Allow building with versions of Glib older than 2.44. (Patch by `cellscape`
  <[me@nooff.info](mailto:me@nooff.info)>.)

## [3] - 2015-05-19
### Added
- Support output in a Mutt-compatible format. This allows using
  `notmuch-addrlookup --mutt` directly in for Mutt's `query_command`
  setting.

### Changed
- Parsing of command line arguments is now done using Glib.

## [2] - 2015-05-12
### Fixed
- Reverse sorting of results, so most frequently contacted addresses are
  sorted first. (Patch by Leonhard Markert.)

## [1] - 2015-01-05
### Added
- First working version of `notmuch-addrlookup`.

[Unreleased]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v7...HEAD
[6]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v6...v7
[6]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v5...v6
[5]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v4...v5
[4]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v3...v4
[3]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v2...v3
[2]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v1...v2
[1]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/a9364d4...v1
