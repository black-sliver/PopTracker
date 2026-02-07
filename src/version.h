#ifndef _VERSION_H
#define _VERSION_H

/// semver <major> of the application
#define APP_VERSION_MAJOR      0
/// semver <minor> of the application
#define APP_VERSION_MINOR     34
/// semver <patch> of the application
#define APP_VERSION_REVISION   0
/// semver "-" <pre-release> and/or "+" <build> of the application
#define APP_VERSION_EXTRA     "-rc1"
/// set to 1 for any pre-release or 0 for release and post-release
#define APP_VERSION_PRERELEASE 1

#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

#define APP_VERSION_STRING XSTR(APP_VERSION_MAJOR.APP_VERSION_MINOR.APP_VERSION_REVISION) APP_VERSION_EXTRA
#define APP_VERSION_TUPLE       APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_REVISION, APP_VERSION_EXTRA
#define APP_VERSION_TUPLE_4I    APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_REVISION,0

#endif // _VERSION_H
