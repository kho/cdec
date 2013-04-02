#ifndef _NEW_DECODER_APP_H_
#define _NEW_DECODER_APP_H_

#include "pipeline.h"
#include "input.h"
#include "context.h"
#include "silent.h"

#include "program_options.h"

#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

namespace NgramCache { void Clear(); }

namespace pipeline {

// print just the --long_opt names suitable for bash compgen
inline void print_options(std::ostream &out, const OptDesc &opts) {
  typedef std::vector< boost::shared_ptr<boost::program_options::option_description> > Ds;
  const Ds &ds = opts.options();
  out << '"';
  for (unsigned i=0;i<ds.size();++i) {
    if (i) out<<' ';
    out<<"--"<<ds[i]->long_name();
  }
  out << '"';
}

inline void ShowBanner() {
  std::cerr << "cdec v1.0 (c) 2009-2011 by Chris Dyer\n";
}


template <class PIPE>
void RunApp(int argc, char *argv[]) {
  using namespace std;
  namespace po = boost::program_options;
  Context context;


  bool show_config;
  vector<string> cfg_files;

  OptDesc opts;
  opts.add_options()
      ("input,i",po::value<string>()->default_value("-"),"Source file")
      ("list_feature_functions,L","List available feature functions")
      ("quiet", "Disable verbose output")
      ("show_config", po::bool_switch(&show_config), "show contents of loaded -c config files.")
      ("show_feature_dictionary", "After decoding the last input, write the contents of the feature dictionary")
      ;
  PIPE::Register(&opts);

  OptDesc clo("Command line options");
  clo.add_options()
      ("config,c", po::value<vector<string> >(&cfg_files), "Configuration file(s) - latest has priority")
      ("help,?", "Print this help message and exit")
      ("usage,u", po::value<string>(), "Describe a feature function type")
      ("compgen", "Print just option names suitable for bash command line completion builtin 'compgen'")
      ;

  OptDesc dconfig_options, dcmdline_options;
  dconfig_options.add(opts);
  dcmdline_options.add(dconfig_options).add(clo);

  VarMap conf;

  argv_minus_to_underscore(argc, argv);
  po::store(parse_command_line(argc, argv, dcmdline_options), conf);
  if (conf.count("compgen")) {
    print_options(cout,dcmdline_options);
    cout << endl;
    exit(0);
  }
  if (conf.count("quiet"))
    SetSilent(true);
  if (!SILENT) ShowBanner();

  if (conf.count("show_config")) // special handling needed because we only want to notify() once.
    show_config=true;
  if (conf.count("config")) {
    typedef vector<string> Cs;
    Cs cs=conf["config"].as<Cs>();
    for (int i=0;i<cs.size();++i) {
      string cfg=cs[i];
      cerr << "Configuration file: " << cfg << endl;
      ReadFile conff(cfg);
      po::store(po::parse_config_file(*conff, dconfig_options), conf);
    }
  }
  po::notify(conf);
  if (show_config && !cfg_files.empty()) {
    cerr<< "\nConfig files:\n\n";
    for (int i=0;i<cfg_files.size();++i) {
      string cfg=cfg_files[i];
      cerr << "Configuration file: " << cfg << endl;
      CopyFile(cfg,cerr);
      cerr << "(end config "<<cfg<<"\n\n";
    }
    cerr <<"Command line:";
    for (int i=0;i<argc;++i)
      cerr<<" "<<argv[i];
    cerr << "\n\n";
  }

  if (conf.count("list_feature_functions")) {
    cerr << "Available feature functions (specify with -F; describe with -u FeatureName):\n";
    ff_registry.DisplayList(); //TODO
    cerr << endl;
    exit(1);
  }

  if (conf.count("usage")) {
    ff_usage(conf["usage"].as<string>());
    exit(0);
  }
  if (conf.count("help")) {
    cout << dcmdline_options << endl;
    exit(0);
  }
  if (conf.count("help") || conf.count("formalism") == 0) {
    cerr << dcmdline_options << endl;
    exit(1);
  }

  PIPE p(conf, &context);

  string source_path = conf["input"].as<string>();
  bool show_feature_dictionary = conf.count("show_feature_dictionary");
  if (!SILENT) cerr << "Reading input from " << ((source_path == "-") ? "STDIN" : source_path.c_str()) << endl;
  ReadFile in_read(source_path);
  istream *in = in_read.stream();
  assert(*in);

  string text;
  while (getline(*in, text)) {
    NgramCache::Clear();   // clear ngram cache for remote LM (if used)
    Input input(text, &context);
    p.Apply(input, &context, Void());
  }

  if (show_feature_dictionary) {
    int num = FD::NumFeats();
    for (int i = 1; i < num; ++i) {
      cout << FD::Convert(i) << endl;
    }
  }
}
} // namespace pipeline

#endif  // _NEW_DECODER_APP_H_
