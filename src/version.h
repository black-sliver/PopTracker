#ifndef _VERSION_H
#define _VERSION_H

#define APP_VERSION_MAJOR      0 /**< semver <major> of PopTracker */
#define APP_VERSION_MINOR     32 /**< semver <minor> of PopTracker */
#define APP_VERSION_REVISION   0 /**< semver <patch> of PopTracker */
#define APP_VERSION_EXTRA     "-rc1" /**< semver "-" <pre-release> and/or "+" <build> of PopTracker */
#define APP_VERSION_PRERELEASE 1 /**< set to 1 for any pre-release or 0 for release and post-release */

#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

#define APP_VERSION_STRING XSTR(APP_VERSION_MAJOR.APP_VERSION_MINOR.APP_VERSION_REVISION) APP_VERSION_EXTRA
#define APP_VERSION_TUPLE       APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_REVISION, APP_VERSION_EXTRA
#define APP_VERSION_TUPLE_4I    APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_REVISION,0

#endif // _VERSION_H
