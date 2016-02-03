# Change Log

All notable changes to this project will be documented in this file. This
project adheres to [Semantic Versioning](http://semver.org).

## v0.35.0 - 2016-02-03
### Changed
- Adopted Semantic Versioning.
- This changelog now follows the [Keep A Changelog](http://keepachangelog.com/)
  format.
- Source code indentation style now uses spaces, never tabs.
- Travis-CI is now used for continuous integration.

## v0.34 - 2016-02-02
### Changed
- Made the code buildable again, supporting Lua versions 5.1, 5.2 and 5.3
- Added rockspec for LuaRocks.

## v0.33 - 2003-04-23
### Fixed
- Removed unneeded `xfree(udata)` in the `__gc` metamethod of the shelve type.
  This caused occassional segmentation faults.

[Unreleased]: https://github.com/aperezdc/lua-shelve/compare/v0.34...HEAD
[v0.34]: https://github.com/aperezdc/lua-shelve/compare/v0.33...v0.34