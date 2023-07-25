#ifndef CONFIG_H
#define CONFIG_H

static const auto FILE_PATH = std::string(__FILE__);
static const auto ROOT_DIR  = FILE_PATH.substr(0, FILE_PATH.rfind("/")) + "/..";

#endif  // #ifndef CONFIG_H
