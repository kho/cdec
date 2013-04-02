#include "input.h"

#include <iostream>
#include <boost/lexical_cast.hpp>

#include "silent.h"
#include "stringlib.h"

using namespace std;

static void ParseTranslatorInputLattice(const string& line, string* input, Lattice* ref) {
  string sref;
  ParseTranslatorInput(line, input, &sref);
  if (sref.size() > 0) {
    assert(ref);
    LatticeTools::ConvertTextOrPLF(sref, ref);
  }
}

namespace pipeline {
Input::Input(std::string &buf, Context *context) {
  ++context->sent_id;
  ProcessAndStripSGML(&buf, &sgml);
  if (sgml.find("id") != sgml.end())
    context->sent_id = boost::lexical_cast<int>(sgml["id"]);
  if (!SILENT) {
    cerr << "\nINPUT: ";
    if (buf.size() < 100)
      cerr << buf << endl;
    else {
      size_t x = buf.rfind(" ", 100);
      if (x == string::npos) x = 100;
      cerr << buf.substr(0, x) << " ..." << endl;
    }
    cerr << "  id = " << context->sent_id << endl;
  }
  ParseTranslatorInputLattice(buf, &to_translate, &ref);
  context->smeta_ptr.reset(new SentenceMetadata(context->sent_id, ref));
  // FIXME: All its use cases now access Input::sgml
  // smeta.sgml_.swap(sgml);
}
} // namespace pipeline
