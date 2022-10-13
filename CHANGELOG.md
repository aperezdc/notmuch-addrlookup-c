# Change Log
All notable changes to this project will be documented in this file.

## [Unreleased]

## [10] - 2022-10-13
### Added
- A new `-c`/`--config` option allows specifying the Notmuch configuration
  file path. (Patch by Lars Haalck
  <[lars.haalck@uni-muenster.de](lars.haalck@uni-muenster.de)>, and follow
  up fix by Bence Ferdinandy <[bence@ferdinandy.com](bence@ferdinandy.com)>).
- A new `-f`/`--format` option supersedes `--mutt`, and also allows using
  `--format=aerc` to be used with the [aerc](https://aerc-mail.org/) user
  agent. (Patch by Bence Ferdinandy <[bence@ferdinandy.com](bence@ferdinandy.com)>).

### Fixed
- Fix more uses of deprecated Notmuch functions.

## [9] - 2017-10-20
### Fixed
- Fix crash in when running queries and the program is build with Notmuch 0.25.
  (Patch by Joshua Krusell <[joshua.krusell@v-dem.net](joshua.krusell@v-dem.net)>).
- Fix conditional compilation when using Notmuch 0.25. (Patch by David Bremner
  <[bremner@debian.org](bremner@debian.org)>).

### Changed
- Do not use the deprecated `notmuch_query_count_messages_st` function
  when using Notmuch 0.25. (Patch by Adam Ruzicka
  <[a.ruzicka@outlook.com](a.ruzicka@outlook.com)>.)

## [8] - 2017-02-07
### Fixed
- Avoid a segmentation fault when `notmuch_message_get_header()` returns
  `NULL`. (Patch by Víctor M. Jáquez
  <[vjaquez@igalia.com](mailto:vjaquez@igalia.com)>).

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

[Unreleased]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v9...HEAD
[10]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v9...v10
[9]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v8...v9
[8]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v7...v8
[7]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v6...v7
[6]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v5...v6
[5]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v4...v5
[4]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v3...v4
[3]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v2...v3
[2]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/v1...v2
[1]: https://github.com/aperezdc/notmuch-addrlookup-c/compare/a9364d4...v1
