#!/usr/bin/env python

APPNAME="rococo"
VERSION="0.0"

import os
import sys
from waflib import Logs
from waflib import Options

pargs = ['--cflags', '--libs']

def options(opt):
    opt.load("compiler_cxx unittest_gtest")
    opt.load('protoc', tooldir=['waf-tools'])
    #opt.load("eclipse")
    opt.add_option('-g', '--use-gxx', dest='cxx',
                   default=False, action='store_true')
    opt.add_option('-c', '--use-clang', dest='clang',
                   default=False, action='store_true')
    opt.add_option('-p', '--enable-profiling', dest='prof',
                   default=False, action='store_true')
    opt.add_option('-d', '--debug', dest='debug',
                   default=False, action='store_true')
    opt.add_option('-t', '--enable-tcmalloc', dest='tcmalloc',
                   default=False, action='store_true')
    opt.add_option('-s', '--enable-rpc-statistics', dest='rpc_s',
                   default=False, action='store_true')
    opt.add_option('-P', '--enable-piece-count', dest='pc',
                   default=False, action='store_true')
    opt.add_option('-C', '--enable-conflict-count', dest='cc',
                   default=False, action='store_true')
    opt.add_option('-r', '--enable-logging', dest='log',
                   default=False, action='store_true')
    opt.add_option('-T', '--enable-txn-stat', dest='txn_stat',
                   default=False, action='store_true')
    opt.add_option('-m', '--mode', dest="mode", 
                   default='', help='mode type', action='store')
    opt.add_option('-l', '--log', dest="log", 
                   default='', help='log level', action='store')
    opt.parse_args();

def configure(conf):
    _choose_compiler(conf)
    _enable_pic(conf)
    conf.load("compiler_cxx unittest_gtest")
    conf.load("python")
    conf.load("boost")
    conf.load('protoc')
    conf.check_python_headers()

    _enable_tcmalloc(conf)
    _enable_cxx11(conf)
    _enable_debug(conf)
    _enable_profile(conf)
    _enable_rpc_s(conf)
    _enable_piece_count(conf)
    _enable_txn_count(conf)
    _enable_conflict_count(conf)
    _enable_log(conf)       #log level
    _enable_mode(conf)      #mode type 
#    _enable_snappy(conf)
    #_enable_logging(conf)


    conf.env.append_value("CXXFLAGS", "-Wno-sign-compare")
    conf.check_boost(lib='system filesystem')

    conf.env.append_value("CXXFLAGS", "-Wno-sign-compare")
    conf.env.append_value('INCLUDES', ['/usr/local/include'])

    conf.env.LIB_PTHREAD = 'pthread'

    conf.check_cfg(package='libzmq', uselib_store='ZMQ', args=pargs)
    conf.check_cfg(package='yaml-cpp', uselib_store='YAML-CPP', args=pargs)

    if sys.platform != 'darwin':
        conf.env.LIB_RT = 'rt'

    # check python modules
    conf.check_python_module('tabulate')

    USED_BOOST_LIBS = ['system', 'filesystem', 'date_time', 'iostreams', 'thread',
                      'regex', 'program_options', 'chrono', 'random']
    conf.check_boost(lib=USED_BOOST_LIBS, mandatory=True)

def build(bld):
    _depend("rrr/pylib/simplerpcgen/rpcgen.py",
            "rrr/pylib/simplerpcgen/rpcgen.g",
            "rrr/pylib/yapps/main.py rrr/pylib/simplerpcgen/rpcgen.g")

#    _depend("rlog/log_service.h", "rlog/log_service.rpc",
#            "bin/rpcgen rlog/log_service.rpc")

    _depend("deptran/rcc_rpc.h deptran/rcc_rpc.py",
            "deptran/rcc_rpc.rpc",
            "bin/rpcgen --python --cpp deptran/rcc_rpc.rpc")

    _depend("old-test/benchmark_service.h", "old-test/benchmark_service.rpc",
            "bin/rpcgen --cpp old-test/benchmark_service.rpc")

    bld.stlib(source=bld.path.ant_glob("rrr/base/*.cpp "
                                       "rrr/misc/*.cpp "
                                       "rrr/rpc/*.cpp"),
              target="rrr",
              includes=". rrr",
              use="PTHREAD")

#    bld.stlib(source=bld.path.ant_glob("rpc/*.cc"), target="simplerpc",
#              includes=". rrr rpc",
#              use="base PTHREAD")

    bld.stlib(source=bld.path.ant_glob("memdb/*.cc"), target="memdb",
              includes=". rrr deptran base",
              use="rrr PTHREAD")

#    bld.stlib(source="rlog/rlog.cc", target="rlog",
#              includes=". rrr rlog rpc",
#              use="simplerpc base PTHREAD")

    bld.shlib(features="pyext",
              source=bld.path.ant_glob("rrr/pylib/simplerpc/*.cc"),
              target="_pyrpc",
              includes=". rrr rrr/rpc",
              use="rrr simplerpc PYTHON")

#    bld.program(source=bld.path.ant_glob("rlog/*.cc", excl="rlog/rlog.cc"),
#                target="rlogserver",
#                includes=". rrr",
#                use="base simplerpc PTHREAD")

    bld.program(source=bld.path.ant_glob("test/*.cc"),
                target="run_tests",
                features="gtest",
                includes=". rrr deptran deptran/ro6 "
                         "deptran/rcc "
                         "deptran/brq "
                         "test memdb",
                uselib="BOOST BOOST_SYSTEM YAML-CPP",
                use="PTHREAD rrr memdb deptran")

    bld.program(source=bld.path.ant_glob("deptran/s_main.cc"),
                target="deptran_server",
                includes=". rrr deptran "
                         "deptran/ro6 deptran/rcc deptran/brq",
                uselib="BOOST BOOST_SYSTEM",
                use="rrr memdb deptran YAML-CPP PTHREAD PROFILER RT")

#    bld.program(source=bld.path.ant_glob("deptran/c_main.cc"),
#                target="deptran_client",
#                includes=". rrr bench deptran deptran/ro6 deptran/rcc deptran/tpl deptran/brq deptran/none",
#                use="rrr memdb deptran YAML-CPP PTHREAD RT")

    bld.program(source="old-test/rpcbench.cc old-test/benchmark_service.cc",
                target="rpcbench",
                includes=". rrr deptran old-test",
                use="rrr PTHREAD")

#    bld.program(source="test/rpc_microbench.cc test/benchmark_service.cc",
#                target="rpc_microbench",
#                includes=". rrr deptran test",
#                use="rrr PTHREAD RT")
#
    bld.stlib(source=bld.path.ant_glob("deptran/*.cc "
                                       "deptran/util/*.cc "
                                       "deptran/ro6/*.cc "
                                       "deptran/rcc/*.cc "
                                       "deptran/brq/*.cc "
                                       "deptran/three_phase/*.cc "
                                       "deptran/tpl/*.cc "
                                       "deptran/none/*.cc "
                                       "bench/tpca/*.cc "
                                       "bench/tpcc/*.cc "
                                       "bench/tpcc_real_dist/*.cc "
                                       "bench/tpcc_dist/*.cc "
                                       "bench/rw_benchmark/*.cc "
                                       "bench/micro/*.cc", excl="deptran/*_main.c*"),
              #use="PTHREAD APR APR-UTIL base simplerpc memdb")
              target="deptran",
              includes=". rrr memdb bench deptran deptran/ro6 deptran/rcc deptran/brq ",
              use="PTHREAD base simplerpc memdb")

    os.system('protoc -I=mpaxos --python_out=script mpaxos/mpaxos.proto')
    bld.stlib(source=bld.path.ant_glob([
                                        'mpaxos/view.cpp', 'mpaxos/proposer.cpp', 'mpaxos/acceptor.cpp', 
                                        'mpaxos/captain.cpp', 'mpaxos/commo.cpp', 'mpaxos/*.proto',
                                        'libzfec/fec.cc']), 
              target="mpaxos",
              includes="mpaxos libzfec",
              use="BOOST PROTOBUF ZMQ YAML-CPP",
              install_path="${PREFIX}/lib")

    for app in bld.path.ant_glob('loli_test/*.cpp'):
        bld(features=['cxx', 'cxxprogram'],
            source = app,
            target = '%s' % (str(app.change_ext('','.cpp'))),
            #cxxflags = ['-std=c++0x'],
            includes="mpaxos libzfec", 
            use="mpaxos",
            ) 


#
# waf helper functions
#
def _choose_compiler(conf):
    # use clang++ as default compiler (for c++11 support on mac)
    if Options.options.cxx:
        os.environ["CXX"] = "g++"
    elif (sys.platform == 'darwin' and "CXX" not in os.environ) or Options.options.clang:
        os.environ["CXX"] = "clang++"
        conf.env.append_value("CXXFLAGS", "-stdlib=libc++")
        conf.env.append_value("LINKFLAGS", "-stdlib=libc++")
    else:
	Logs.pprint("PINK", "use system default compiler")

def _enable_rpc_s(conf):
    if Options.options.rpc_s:
        conf.env.append_value("CXXFLAGS", "-DRPC_STATISTICS")
        Logs.pprint("PINK", "RPC statistics enabled")

def _enable_piece_count(conf):
    if Options.options.pc:
        conf.env.append_value("CXXFLAGS", "-DPIECE_COUNT")
        Logs.pprint("PINK", "Piece count enabled")

def _enable_txn_count(conf):
    if Options.options.txn_stat:
        conf.env.append_value("CXXFLAGS", "-DTXN_STAT")
        Logs.pprint("PINK", "Txn stat enabled")

def _enable_conflict_count(conf):
    if Options.options.cc:
        conf.env.append_value("CXXFLAGS", "-DCONFLICT_COUNT")
        Logs.pprint("PINK", "Conflict count enabled")

def _enable_logging(conf):
    #if Options.options.log:
    conf.check(compiler='cxx', lib='aio', mandatory=True, uselib_store="AIO")
    conf.env.append_value("CXXFLAGS", "-DRECORD")
    conf.env.append_value("LINKFLAGS", "-laio")
    Logs.pprint("PINK", "Logging enabled")

def _enable_snappy(conf):
    Logs.pprint("PINK", "google snappy enabled")
    conf.env.append_value("LINKFLAGS", "-Wl,--no-as-needed")
    conf.env.append_value("LINKFLAGS", "-lsnappy")
    conf.env.append_value("LINKFLAGS", "-Wl,--as-needed")

def _enable_tcmalloc(conf):
    if Options.options.tcmalloc:
        Logs.pprint("PINK", "tcmalloc enabled")
        conf.env.append_value("LINKFLAGS", "-Wl,--no-as-needed")
        conf.env.append_value("LINKFLAGS", "-ltcmalloc")
        conf.env.append_value("LINKFLAGS", "-Wl,--as-needed")

def _enable_pic(conf):
    conf.env.append_value("CXXFLAGS", "-fPIC")
    conf.env.append_value("LINKFLAGS", "-fPIC")

def _enable_cxx11(conf):
    Logs.pprint("PINK", "C++11 features enabled")
    if sys.platform == "darwin":
        conf.env.append_value("CXXFLAGS", "-stdlib=libc++")
        conf.env.append_value("LINKFLAGS", "-stdlib=libc++")
    conf.env.append_value("CXXFLAGS", "-std=c++0x")

def _enable_profile(conf):
    if Options.options.prof:
        Logs.pprint("PINK", "CPU profiling enabled")
        conf.env.append_value("CXXFLAGS", "-DCPU_PROFILE")
        conf.env.LIB_PROFILER = 'profiler'

def _enable_debug(conf):
    if Options.options.debug:
        Logs.pprint("PINK", "Debug support enabled")
        conf.env.append_value("CXXFLAGS", "-Wall -pthread -O0 -DNDEBUG -g "
                "-ggdb -DLOG_DEBUG -rdynamic -fno-omit-frame-pointer".split())
    else:
        conf.env.append_value("CXXFLAGS", "-Wall -pthread -O2 -DNDEBUG".split())

def _properly_split(args):
    if args == None:
        return []
    else:
        return args.split()

def _depend(target, source, action):
    target = _properly_split(target)
    source = _properly_split(source)
    for s in source:
        if not os.path.exists(s):
            Logs.pprint('RED', "'%s' not found!" % s)
            exit(1)
    for t in target:
        if not os.path.exists(t):
            _run_cmd(action)
    if not target or min([os.stat(t).st_mtime for t in target]) < max([os.stat(s).st_mtime for s in source]):
        _run_cmd(action)

def _run_cmd(cmd):
    Logs.pprint('PINK', cmd)
    os.system(cmd)

def _enable_log(conf):
    if Options.options.log == 'trace':
        Logs.pprint("PINK", "Log level set to trace")
        conf.env.append_value("CFLAGS", "-DLOG_LEVEL=6")
        conf.env.append_value("CXXFLAGS", "-DLOG_LEVEL=6")
    elif Options.options.log == 'debug':
        Logs.pprint("PINK", "Log level set to debug")
        conf.env.append_value("CFLAGS", "-DLOG_LEVEL=5")
        conf.env.append_value("CXXFLAGS", "-DLOG_LEVEL=5")
    elif Options.options.log == 'info':
        Logs.pprint("PINK", "Log level set to info")
        conf.env.append_value("CFLAGS", "-DLOG_LEVEL=4")
        conf.env.append_value("CXXFLAGS", "-DLOG_LEVEL=4")
    elif Options.options.log == '':
        pass
    else:
        Logs.pprint("PINK", "unsupported log level")
#    if os.getenv("DEBUG") == "1":

def _enable_mode(conf):
    if Options.options.mode == 'RS':
        Logs.pprint("PINK", "Mode type set to RS")
        conf.env.append_value("CFLAGS", "-DMODE_TYPE=1")
        conf.env.append_value("CXXFLAGS", "-DMODE_TYPE=1")
    elif Options.options.mode == 'E':
        Logs.pprint("PINK", "Mode type set to Epaxos")
        conf.env.append_value("CFLAGS", "-DMODE_TYPE=2")
        conf.env.append_value("CXXFLAGS", "-DMODE_TYPE=2")
    elif Options.options.mode == 'RSII':
        Logs.pprint("PINK", "Mode type set to RSII")
        conf.env.append_value("CFLAGS", "-DMODE_TYPE=3")
        conf.env.append_value("CXXFLAGS", "-DMODE_TYPE=3")
    elif Options.options.mode == '':
        pass
    else:
        Logs.pprint("PINK", "unsupported Mode type")
