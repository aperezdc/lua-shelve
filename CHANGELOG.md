# Change Log

All notable changes to this project will be documented in this file. This
project adheres to [Semantic Versioning](http://semver.org).

## [Unreleased]

## [v0.35.3] - 2020-01-14
### Fixed
- Make it possible to build again with LuaJIT 2.1

## [v0.35.2] - 2020-01-14
### Changed
- The `__tostring` metamethod for a shelve userdata no longer includes the
  file name of the shelve in the values it returns.

### Fixed
- The module can now be built and used with LuaJIT 2.0 and 2.1.

## [v0.35.1] - 2016-04-18
### Changed
- The `__call` metamethod for a shelve userdata now returns an iterator
  function instead of a table with the keys.

## [v0.35.0] - 2016-02-03
### Changed
- Adopted Semantic Versioning.
- This changelog now follows the [Keep A Changelog](http://keepachangelog.com/)
  format.
- Source code indentation style now uses spaces, never tabs.
- Travis-CI is now used for continuous integration.

## [v0.34] - 2016-02-02
### Changed
- Made the code buildable again, supporting Lua versions 5.1, 5.2 and 5.3
- Added rockspec for LuaRocks.

## v0.33 - 2003-04-23
### Fixed
- Removed unneeded `xfree(udata)` in the `__gc` metamethod of the shelve type.
  This caused occassional segmentation faults.

[Unreleased]: https://github.com/aperezdc/lua-shelve/compare/v0.35.3...HEAD
[v0.35.3]: https://github.com/aperezdc/lua-shelve/compare/v0.35.2...v0.35.3
[v0.35.2]: https://github.com/aperezdc/lua-shelve/compare/v0.35.1...v0.35.2
[v0.35.1]: https://github.com/aperezdc/lua-shelve/compare/v0.35.0...v0.35.1
[v0.35.0]: https://github.com/aperezdc/lua-shelve/compare/v0.34...v0.35.0
[v0.34]: https://github.com/aperezdc/lua-shelve/compare/v0.33...v0.34
