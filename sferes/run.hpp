//| This file is a part of the sferes2 framework.
//| Copyright 2009, ISIR / Universite Pierre et Marie Curie (UPMC)
//| Main contributor(s): Jean-Baptiste Mouret, mouret@isir.fr
//|
//| This software is a computer program whose purpose is to facilitate
//| experiments in evolutionary computation and evolutionary robotics.
//|
//| This software is governed by the CeCILL license under French law
//| and abiding by the rules of distribution of free software.  You
//| can use, modify and/ or redistribute the software under the terms
//| of the CeCILL license as circulated by CEA, CNRS and INRIA at the
//| following URL "http://www.cecill.info".
//|
//| As a counterpart to the access to the source code and rights to
//| copy, modify and redistribute granted by the license, users are
//| provided only with a limited warranty and the software's author,
//| the holder of the economic rights, and the successive licensors
//| have only limited liability.
//|
//| In this respect, the user's attention is drawn to the risks
//| associated with loading, using, modifying and/or developing or
//| reproducing the software by the user in light of its specific
//| status of free software, that may mean that it is complicated to
//| manipulate, and that also therefore means that it is reserved for
//| developers and experienced professionals having in-depth computer
//| knowledge. Users are therefore encouraged to load and test the
//| software's suitability as regards their requirements in conditions
//| enabling the security of their systems and/or data to be ensured
//| and, more generally, to use and operate it in the same conditions
//| as regards security.
//|
//| The fact that you are presently reading this means that you have
//| had knowledge of the CeCILL license and that you accept its terms.

#ifndef _RUN_HPP_
#define _RUN_HPP_

#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/foreach.hpp>

// this is UGLY hack to avoid the name clash between
// the C function stat() [man 2 stat] and our stat::
// namespace
// (asio include stat but we do not use it...)
#define stat stat_test
#include <boost/asio/signal_set.hpp>
#undef stat

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <sferes/parallel.hpp>
#include <sferes/dbg/dbg.hpp>

namespace sferes {
  template<typename EA>
  static void _sig_handler(EA& ea,
                    const boost::system::error_code& error,
                    int signal_number) {
  if (!error) {
      std::cout<<"received signal:"<<signal_number<<std::endl;
      std::cout<<"ea.gen():"<<ea.gen()<<std::endl;
      ea.write();
      ea.stop();
   }
   else
    std::cerr<<"error in sig handler:" << error.message() << std::endl;
  }

  template<typename Ea>
  static void run_ea(int argc,
                     char **argv,
                     Ea& ea,
                     const typename Ea::phen_t::fit_t& fit_proto,
                     const boost::program_options::options_description& add_opts =
                       boost::program_options::options_description(),
                     bool init_rand = true) {
    ea.set_fit_proto(fit_proto);

    // handler (dump in case of sigterm/sigkill)
    boost::asio::io_service io_service;
    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM, SIGQUIT);
    signals.async_wait(boost::bind(_sig_handler<Ea>, boost::ref(ea), _1, _2));
    boost::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(io_service));
    boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
    t.detach();

    // command-line options
    namespace po = boost::program_options;
    std::cout<<"sferes2 version: "<<VERSION<<std::endl;
    if (init_rand) {
      time_t t = time(0) + ::getpid();
      std::cout<<"seed: " << t << std::endl;
      srand(t);
    }
    po::options_description desc("Available sferes2 options");
    desc.add(add_opts);
    desc.add_options()
    ("help,h", "produce help message")
    ("stat,s", po::value<int>(), "statistic number")
    ("out,o", po::value<std::string>(), "output file")
    ("number,n", po::value<int>(), "number in stat")
    ("load,l", po::value<std::string>(), "load a result file")
    ("resume,r", po::value<std::string>(), "load a full state and resume the algorithm")
    ("verbose,v", po::value<std::vector<std::string> >()->multitoken(),
     "verbose output, available default streams : all, ea, fit, phen, trace")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return;
    }
    if (vm.count("verbose")) {
      dbg::init();
      std::vector<std::string> streams =
        vm["verbose"].as<std::vector<std::string> >();
      attach_ostream(dbg::warning, std::cout);
      attach_ostream(dbg::error, std::cerr);
      attach_ostream(dbg::info, std::cout);
      bool all = std::find(streams.begin(), streams.end(), "all") != streams.end();
      bool trace = std::find(streams.begin(), streams.end(), "trace") != streams.end();
      if (all) {
        streams.push_back("ea");
        streams.push_back("fit");
        streams.push_back("phen");
        streams.push_back("eval");
      }
      BOOST_FOREACH(const std::string& s, streams) {
        dbg::enable(dbg::all, s.c_str(), true);
        dbg::attach_ostream(dbg::info, s.c_str(), std::cout);
        if (trace)
          dbg::attach_ostream(dbg::tracing, s.c_str(), std::cout);
      }
      if (trace)
        attach_ostream(dbg::tracing, std::cout);
    }

    parallel::init();
    if (vm.count("load")) {
      ea.load(vm["load"].as<std::string>());

      if (!vm.count("out")) {
        std::cerr<<"You must specifiy an out file"<<std::endl;
        return;
      } else {
        int stat = 0;
        int n = 0;
        if (vm.count("stat"))
          stat = vm["stat"].as<int>();
        if (vm.count("number"))
          n = vm["number"].as<int>();
        std::ofstream ofs(vm["out"].as<std::string>().c_str());
        ea.show_stat(stat, ofs, n);
      }
    } else if (vm.count("resume")) {
      ea.resume(vm["resume"].as<std::string>());
    } else {
      ea.run();
    }
  }

  template<typename Ea>
  static void run_ea(int argc,
                     char **argv,
                     Ea& ea,
                     const boost::program_options::options_description& add_opts =
                       boost::program_options::options_description(),
                     bool init_rand = true) {
    run_ea(argc, argv, ea, typename Ea::phen_t::fit_t(), add_opts, init_rand);
  }

}


#endif
