#!/usr/bin/python3

import os
from posixpath import basename
import sys
import json
import glob
import filecmp
import subprocess
import time
import signal
import configparser
import re
import shutil
import functools
import tempfile
import random
import ctypes
import resource
import struct

from queue_manager import QueueManager
from limiter import setlimits, RUNNING_PROCESSES, SHUTDOWN
import hybrid


class Executor(object):

    def __init__(self, binary, input, output_dir, binary_args,
                 debug=None, afl=None, timeout=None, input_fixed_name=None,
                 keep_run_dirs=False, cache_dir=None, hybrid_mode=False,
                 generate_hybrid_conf=None, forkserver=False, queue_mode=None):

        if not os.path.exists(binary):
            sys.exit('ERROR: invalid binary')
        self.binary = os.path.abspath(binary)

        self.binary_args = binary_args
        self.testcase_from_stdin = '@@' not in self.binary_args
        
        self.hybrid_mode = hybrid_mode
        self.hybrid_conf = os.path.abspath(output_dir) + "/hybrid.conf" if generate_hybrid_conf is None else generate_hybrid_conf
        if self.hybrid_mode:
            hybrid.build_conf(self.hybrid_conf, self.binary,
                              os.path.abspath(output_dir) + "/hybrid.log" if generate_hybrid_conf is None else None)
            print("Done generating config.")
        
        if generate_hybrid_conf:
            sys.exit(0)

        if not os.path.exists(output_dir):
            sys.exit('ERROR: invalid working directory')
        self.output_dir = os.path.abspath(output_dir)

        if not os.path.exists(input):
            sys.exit('ERROR: invalid input')
        self.input = os.path.abspath(input)

        if afl:
            if not os.path.exists(afl):
                sys.exit('ERROR: invalid AFL workdir')
            self.afl = os.path.abspath(afl)
        else:
            self.afl = None

        self.afl_processed_testcases = set()
        self.afl_alt_processed_testcases = set()
        self.timeout_testcases = set()
        self.debug = debug
        self.tick_count = 0
        self.timeout = timeout
        self.input_fixed_name = input_fixed_name
        self.keep_run_dirs = keep_run_dirs
        self.work_dir = self.__get_root_dir() + "/workdir/"
        self.cex_cache_dir = cache_dir
        if self.cex_cache_dir is None:
            self.cex_cache_dir = self.__get_root_dir() + "/cache/"
        self.avoid_cache_dir = self.__get_root_dir() + "/avoid/"
        self.queue_manager = QueueManager(
            self.__get_root_dir(), self.binary, self.binary_args, 
            self.hybrid_mode, self.hybrid_conf, self.afl,
            mode=queue_mode
        )
        
        self.input_file = self.__get_work_dir() + "/input_file"
        
        self.forkserver = None
        if forkserver:
            print("Starting forkserver of the runner...")
            
            self.pipe_read_path = self.__get_root_dir() + "/pipe_read"
            self.pipe_write_path = self.__get_root_dir() + "/pipe_write"
            os.mkfifo(self.pipe_read_path)
            os.mkfifo(self.pipe_write_path)
            
            self.forkserver_file_done = self.__get_root_dir() + "/forkserver_child_status"
            self.forkserver_file_pid = self.__get_root_dir() + "/forkserver_child_pid"
            env = os.environ.copy()
            env['SYMFUSION_PATH_FORKSERVER_DONE'] = self.forkserver_file_done
            env['SYMFUSION_FORKSERVER_PIPE_WRITE'] = self.pipe_read_path
            env['SYMFUSION_FORKSERVER_PIPE_READ'] = self.pipe_write_path
            env['SYMFUSION_PATH_FORKSERVER_PID'] = self.forkserver_file_pid
            self.forkserver = True
            self.forkserver, _, _ = self.prepare_run(env, run_dir=self.__get_work_dir(), testcase=self.input_file)
            RUNNING_PROCESSES.append(self.forkserver)

    def __get_root_dir(self):
        if not os.path.exists(self.output_dir):
            os.system('mkdir ' + self.output_dir)
        return self.output_dir

    def __get_cex_cache_dir(self):
        if not os.path.exists(self.cex_cache_dir):
            os.system('mkdir ' + self.cex_cache_dir)
        return self.cex_cache_dir
    
    def __get_avoid_cache_dir(self):
        if not os.path.exists(self.avoid_cache_dir):
            os.system('mkdir ' + self.avoid_cache_dir)
        return self.avoid_cache_dir
    
    def __get_work_dir(self):
        if not os.path.exists(self.work_dir):
            os.system('mkdir ' + self.work_dir)
        return self.work_dir

    def __get_run_dir(self):
        root_dir = self.__get_root_dir()

        # status file
        status_file = root_dir + '/status'
        if not os.path.exists(status_file):
            os.system('echo 0 > ' + status_file)

        # get id for next run
        with open(status_file, 'r') as sf:
            run_id = sf.read()
            run_id = int(run_id)
            assert(run_id >= 0)

        # run dir
        run_dir = root_dir + '/symfusion-%08d' % run_id
        os.system('mkdir ' + run_dir)

        # increment id for the next run
        with open(status_file, 'w') as sf:
            sf.write(str(run_id + 1))

        return run_dir, run_id
    
    def prepare_run(self, env, run_dir, testcase, output_log=None):
        
        # env['SYMFUSION_CEX_CACHE_DIR'] = self.__get_cex_cache_dir()
        # env['SYMFUSION_AVOID_CACHE_DIR'] = self.__get_avoid_cache_dir()
        env['SYMCC_AFL_COVERAGE_MAP'] = self.__get_root_dir() + "/map"
        env['SYMCC_OUTPUT_DIR'] = run_dir
        if not self.testcase_from_stdin:
            env['SYMCC_INPUT_FILE'] = testcase
        else:
            env['SYMCC_INPUT_STDIN_FILE'] = testcase
            
        p_args = []
        if self.debug == "gdb":
            p_args.append("gdb")
            
        # p_args += ['perf', 'record', '--call-graph', 'dwarf']

        args = self.binary_args[:]
        if self.hybrid_mode:
            p_args, args, env = hybrid.prepare(
                self.binary, self.binary_args, p_args, args, env, self.hybrid_conf)
        else:
            p_args += [self.binary]
            
        if not self.testcase_from_stdin:
            for k in range(len(args)):
                if args[k] == '@@':
                    args[k] = testcase

        if self.debug != "gdb":
            p_args += args
            
        # print(p_args)
            
        start = time.time()
        p = subprocess.Popen(
            p_args,
            stdout=None if self.debug else (output_log if output_log is not None else subprocess.DEVNULL),
            stderr=None if self.debug else (output_log if output_log is not None else subprocess.DEVNULL),
            stdin=None, # subprocess.DEVNULL,
            cwd=run_dir,
            env=env,
            preexec_fn=setlimits,
            start_new_session=True, 
            # bufsize=1  #if self.debug == 'gdb' else -1,
            # universal_newlines=True if self.debug == 'gdb_tracer' else False
        )
        
        if self.forkserver:
            self.pipe_write = open(self.pipe_write_path, 'wb')
            self.pipe_read = open(self.pipe_read_path, 'rb')
        
        return p, start, args

    def fuzz_one(self, testcase):

        global RUNNING_PROCESSES
        self.__check_shutdown_flag()

        run_dir, run_id = self.__get_run_dir()
        print('Working directory: %s => %s' % (os.path.basename(self.__get_work_dir()[:-1]), os.path.basename(run_dir)))

        os.system("cp " + testcase + " " + self.input_file)

        if self.forkserver is None:
            env = os.environ.copy()
            output_log_name = run_dir + '/output.log'
            output_log = open(output_log_name, "w")
            p, start, args = self.prepare_run(env, run_dir=self.__get_work_dir(), testcase=self.input_file, output_log=output_log)
            RUNNING_PROCESSES.append(p)
        else:
            start = time.time()
            if os.path.exists(self.forkserver_file_done):
                os.unlink(self.forkserver_file_done)
            if os.path.exists(self.forkserver_file_pid):
                os.unlink(self.forkserver_file_pid)
            self.pipe_write.write(bytes([23]))
            self.pipe_write.flush()
            
        print("Running...")

        if self.debug == 'gdb':
            print("\nGDB COMMAND:\n\trun %s%s\n" % (' '.join(args),
                                                    (" < " + testcase if self.testcase_from_stdin else "")))

        if self.forkserver is None:
            # print("Waiting...")
            try:
                p.wait(self.timeout)
            except subprocess.TimeoutExpired:
                print('[SymFusion] Sending SIGINT to target binary.')
                p.send_signal(signal.SIGINT)
                try:
                    p.wait(1)
                except subprocess.TimeoutExpired:
                    print('[SymFusion] Sending SIGKILL to target binary.')
                    p.send_signal(signal.SIGKILL)
                    p.wait()
            if p in RUNNING_PROCESSES:
                RUNNING_PROCESSES.remove(p)
            if output_log:
                output_log.close()
            p_return_code = p.returncode
        else:
            wait_time = 0
            while not os.path.exists(self.forkserver_file_pid):
                time.sleep(0.0005)
                wait_time += 0.0005
            p_pid = None
            while p_pid is None:          
                try:
                    data = open(self.forkserver_file_pid, 'rb').read()
                    p_pid = struct.unpack("I", data)[0]
                except:
                    pass
            print("Child PID: %d" % p_pid)
            while not os.path.exists(self.forkserver_file_done):
                time.sleep(0.0005)
                wait_time += 0.0005
                if self.timeout and wait_time > self.timeout:
                    print("Killing child: %d" % p_pid)
                    os.system("kill -9 %s" % p_pid)
                    break
            data = self.pipe_read.read(1)
            p_return_code = None
            while p_return_code is None:
                try:
                    data = open(self.forkserver_file_done, 'rb').read()
                    p_return_code = struct.unpack("I", data)[0]
                except:
                    # print("Failed to read status code of the child. Retrying...")
                    time.sleep(0.0005)
          
        end = time.time()
        print("Completed (time=%.3f)" % (end-start))

        if p_return_code != 0:
            returncode_str = "(SIGSEGV)" if p_return_code == -11 else ""
            print("ERROR: target binary has returned code %d %s" %
                  (p_return_code, returncode_str))

        # check new test cases
        if not self.debug:
            # tracer_runner_time, tracer_comparer_time, _ = (0, 0, 0) # 
            tracer_runner_time, tracer_comparer_time, _ = self.queue_manager.add_from_dir(self.__get_work_dir(), testcase)
            tracer_time = time.time()
        else:
            tracer_runner_time = 0
            tracer_comparer_time = 0
            tracer_time = end

        if self.keep_run_dirs:
            os.system("mv %s/* %s" % (self.__get_work_dir(), run_dir))
            os.system("rm %s/%s" % (run_dir, os.path.basename(self.input_file)))
            os.system("cp " + testcase + " " + run_dir)
        else:
            print("Removing %s..." % run_dir)
            shutil.rmtree(run_dir)
        
        shutil.rmtree(self.__get_work_dir())
            
        return (end-start), (tracer_time-end), tracer_runner_time, tracer_comparer_time

    def tick(self):
        self.tick_count += 1
        return self.tick_count - 1

    @property
    def cur_input(self):
        if self.input_fixed_name:
            return self.__get_root_dir() + '/' + self.input_fixed_name
        else:
            return self.__get_root_dir() + '/.cur_input'

    def __check_shutdown_flag(self):
        if SHUTDOWN:
            sys.exit("Forcefully terminating...")

    def run(self):

        if not self.debug or self.debug == "tracer":
            print("\nEvaluating seed inputs:")
            tracer_start = time.time()
            input_dir = self.input
            if os.path.isdir(self.input):
                tmp_dir = tempfile.TemporaryDirectory()
                input_dir = tmp_dir.name
                tracer_count = 0
                for seed in sorted(glob.glob(self.input + "/*")):
                    os.system("cp %s %s" % (seed, tmp_dir.name))
                    tracer_count += 1 
                    if self.debug == "tracer":
                        if tracer_count == 500:
                            break
            else:
                tmp_dir = tempfile.TemporaryDirectory()
                os.system("cp %s %s" % (self.input, tmp_dir.name))
                input_dir = tmp_dir.name
                tracer_count = 1
            self.queue_manager.add_from_dir(input_dir, "seed")
                
        if self.debug == "tracer":
            total = time.time() - tracer_start
            average = total / tracer_count
            print("Tracer: cumulative=%f secs, average=%f secs, count=%d" % (total, average, tracer_count))
            sys.exit(0)

        total_runner_time = 0
        total_tracer_time = 0
        total_pick_time = 0
        total_run_time = 0

        self.__check_shutdown_flag()
        if not self.debug:
            testcase, tracer_run_time, tracer_comp_time = self.queue_manager.pick_testcase()
            total_tracer_time += tracer_run_time + tracer_comp_time
        else:
            testcase = self.input
            
        runs = 0
        start_loop = time.time()
        while testcase:
            runs += 1
            start = time.time()
            runner_time, tracer_time, tracer_run_time, tracer_comp_time = self.fuzz_one(testcase)
            end = time.time()
            total_runner_time += runner_time
            total_tracer_time += tracer_time
            print("Run took %.2f secs [runner=%.2f, tracer=%.2f, tracer_run=%.2f, tracer_comp=%.2f, total_tracer=%.2f, total_runner=%.2f, avg_total_runner=%.3f, all_time=%.2f, pick_time=%.2f]" 
                        % (end-start, runner_time, tracer_time, tracer_run_time, tracer_comp_time, total_tracer_time, total_runner_time, total_runner_time / runs, end-start_loop, total_pick_time))
            total_run_time += end-start

            start = time.time()
            if self.debug:
                return
            self.__check_shutdown_flag()
            testcase = None
            while testcase is None: 
                testcase, tracer_run_time, tracer_comp_time = self.queue_manager.pick_testcase()
                total_tracer_time += tracer_run_time + tracer_comp_time
                if testcase is None and self.afl:
                    time.sleep(0.5)
                else:
                    break
            self.__check_shutdown_flag()
            end = time.time()
            total_pick_time += end - start

            print("total_run=%.2f, total_pick=%.2f, all_time=%.2f" % (total_run_time, total_pick_time, end-start_loop))

        print("\n[SymFusion] no more testcases. Finishing.\n")
