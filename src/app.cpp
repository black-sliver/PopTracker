#include "app.h"


#ifdef __EMSCRIPTEN__
// static emscripten app instance
App* App::_emInstance;
#endif
