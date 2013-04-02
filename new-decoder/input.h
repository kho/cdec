#ifndef _NEW_DECODER_INPUT_H_
#define _NEW_DECODER_INPUT_H_

#include "context.h"

#include <map>
#include <string>

#include "lattice.h"

namespace pipeline {
class Input {
 public:
  std::map<std::string, std::string> sgml;
  std::string to_translate;
  Lattice ref;

  Input(std::string &, Context *context);
};
} // namespace pipeline

#endif  // _NEW_DECODER_INPUT_H_
