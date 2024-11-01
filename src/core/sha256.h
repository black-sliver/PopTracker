#ifndef _CORE_SHA256_H
#define _CORE_SHA256_H

#include <string>
#include "fs.h"

std::string SHA256_File(const fs::path& file);

#endif // _CORE_SHA256_H
