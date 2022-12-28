#!/usr/bin/python3

import os
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
import struct
import posix

from limiter import setlimits, RUNNING_PROCESSES
import hybrid

class PathTracer(object):

    def __init__(self, binary, binary_args, root_dir,
                 main_edge_bitmap=None, all_edge_bitmap=None,
                 main_path_bitmap=None, all_path_bitmap=None,
                 bitmap_size=65536, hybrid_mode=False,
                 hybrid_conf=None):
        self.binary = binary
        self.binary_args = binary_args
        self.testcase_from_stdin = '@@' not in self.binary_args
        self.root_dir = root_dir
        self.tracer_dir = self.root_dir + "/tracer"
        os.system("mkdir %s" % self.tracer_dir)
        self.bitmap_size = bitmap_size
        self.hybrid_mode = hybrid_mode
        self.hybrid_conf = hybrid_conf
        # unique edges
        self.main_edge_bitmap = main_edge_bitmap if main_edge_bitmap else root_dir + \
            "/bitmap_edge_main"
        self.all_edge_bitmap = all_edge_bitmap if all_edge_bitmap else root_dir + "/bitmap_edge_all"
        # unique path
        self.main_path_bitmap = main_path_bitmap if main_path_bitmap else root_dir + \
            "/bitmap_path_main"
        self.all_path_bitmap = all_path_bitmap if all_path_bitmap else root_dir + "/bitmap_path_all"
        
        self.initialize_bitmap(self.main_edge_bitmap)
        self.initialize_bitmap(self.all_edge_bitmap)
        
        self.pipe_read_path = self.tracer_dir + "/pipe_read"
        self.pipe_write_path = self.tracer_dir + "/pipe_write"
        os.mkfifo(self.pipe_read_path)
        os.mkfifo(self.pipe_write_path)
    
        # print("Running path tracer on %s" % os.path.basename(testcase))

        # bitmaps for the current testcase
        self.virgin_main_edge_bitmap = self.tracer_dir + "/bitmap_virgin_edge_main"
        self.virgin_all_edge_bitmap = self.tracer_dir + "/bitmap_virgin_edge_all"
        self.virgin_main_path_bitmap = self.tracer_dir + "/bitmap_virgin_path_main"
        self.virgin_all_path_bitmap = self.tracer_dir + "/bitmap_virgin_path_all"
        self.path_tracer_file_done = self.tracer_dir + "/done"
        self.path_tracer_file_pid = self.tracer_dir + "/pid"

        testcase = self.tracer_dir + "/input"
        self.tracer_testcase = testcase
        
        # os.system("cp %s %s" % ("/mnt/ssd/symbolic/symfusion/tests/objdump/small_exec.elf", self.tracer_testcase))

        env = os.environ.copy()
        env['SYMFUSION_BITMAP_SIZE'] = str(self.bitmap_size)
        env['SYMFUSION_BITMAP_TRACE_EDGE_MAIN'] = self.main_edge_bitmap
        env['SYMFUSION_BITMAP_TRACE_EDGE_ALL'] = self.all_edge_bitmap
        env['SYMFUSION_BITMAP_VIRGIN_EDGE_MAIN'] = self.virgin_main_edge_bitmap
        env['SYMFUSION_BITMAP_VIRGIN_EDGE_ALL'] = self.virgin_all_edge_bitmap
        env['SYMFUSION_BITMAP_VIRGIN_PATH_MAIN'] = self.virgin_main_path_bitmap
        env['SYMFUSION_BITMAP_VIRGIN_PATH_ALL'] = self.virgin_all_path_bitmap
        env['SYMFUSION_FORKSERVER_PIPE_WRITE'] = self.pipe_read_path
        env['SYMFUSION_FORKSERVER_PIPE_READ'] = self.pipe_write_path
        env['SYMFUSION_PATH_TRACER'] = "1"
        env['SYMFUSION_PATH_FORKSERVER_DONE'] = self.path_tracer_file_done
        env['SYMFUSION_PATH_FORKSERVER_PID'] = self.path_tracer_file_pid
        env['SYMFUSION_HYBRID'] = "1"
        # env['SYMCC_NO_SYMBOLIC_INPUT'] = "1"
        env['SYMCC_OUTPUT_DIR'] = self.tracer_dir
        if not self.testcase_from_stdin:
            env['SYMCC_INPUT_FILE'] = testcase
        else:
            env['SYMCC_INPUT_STDIN_FILE'] = testcase
            
        # env["SYMFUSION_TRACER_SKIP_DUMP"] = "1"

        use_gdb = False
        p_args = []

        if use_gdb:
            p_args += ['gdb']

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
                    
        if not use_gdb:
            p_args += args
            pass
        else:
            print("\nGDB COMMAND:\n\trun %s%s\n" % (' '.join(args),
                                                    (" < " + testcase if self.testcase_from_stdin else "")))

        if False:
            env['SYMCC_NO_SYMBOLIC_INPUT'] = "1"
            p_args[0] = "/mnt/ssd/symbolic/symqemu-forkserver/x86_64-linux-user/symqemu-x86_64"
            f = glob.glob("/home/symbolic/qemu-hybrid-test/tests/*/%s.symqemu" % os.path.basename(binary).replace(".symfusion", ""))
            p_args[1] = f[0]
            print(p_args)
            main_address = subprocess.check_output("objdump -d %s | grep \"<main>\" | awk '{ print $1 }'" % f[0], shell=True)
            env['FORKSERVER_MAIN'] = main_address.decode('ascii')
            print("Main address: %s" % main_address.decode('ascii'))
        
        start = time.time()
        p = subprocess.Popen(
            p_args,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            stdin=subprocess.PIPE if self.testcase_from_stdin and use_gdb is False else None,
            cwd=self.tracer_dir,
            env=env,
            preexec_fn=setlimits
        )
        
        self.tracer_process = p
        self.tracer_pid = p.pid
        RUNNING_PROCESSES.append(p)
        
        self.pipe_write = open(self.pipe_write_path, 'wb')
        self.pipe_read = open(self.pipe_read_path, 'rb')
        
        """
        p.wait()
        sys.exit(0)
        end = time.time()
        total = end-start
        """
        
    def run(self, testcase_run):
        
        # print("Executing tracer over %s" % os.path.basename(testcase_run))
        
        start = time.time()
        if os.path.exists(self.virgin_main_edge_bitmap):
            os.unlink(self.virgin_main_edge_bitmap)
        if os.path.exists(self.virgin_all_edge_bitmap):
            os.unlink(self.virgin_all_edge_bitmap)
        if os.path.exists(self.virgin_main_path_bitmap):
            os.unlink(self.virgin_main_path_bitmap)
        if os.path.exists(self.virgin_all_path_bitmap):
            os.unlink(self.virgin_all_path_bitmap)
        if os.path.exists(self.path_tracer_file_done):
            os.unlink(self.path_tracer_file_done)

        os.system("cp %s %s" % (testcase_run, self.tracer_testcase))
        # os.system("cp %s %s" % ("/mnt/ssd/symbolic/symfusion/tests/objdump/small_exec.elf", self.tracer_testcase))
        # os.system("cp %s %s" % ("/bin/true", self.tracer_testcase))

        # print("Opening pipe")
        self.pipe_write.write(bytes([23]))
        self.pipe_write.flush()
        # print("Waiting for termination...")
        
        data = self.pipe_read.read(1)
        
        # while not os.path.exists(self.path_tracer_file_done):
        #    time.sleep(0.0005)
        
        end = time.time()
        
        # return None, None, None, None, end-start, end-end, 0
            
        status_code = self.read_values_report(self.path_tracer_file_done, 4)
        status_code = status_code[0] if len(status_code) > 0 else None
        
        # print("Path tracer: run=%.3f status_code=%s" % (end-start, status_code))
        
        # print("Total time: run=%f total=%f" % (total / repeat, total))
        # self.tracer_process.send_signal(signal.SIGTERM)
        # sys.exit(0)

        unique_main_edges = self.read_values_report(self.virgin_main_edge_bitmap, 2)
        unique_all_edges = self.read_values_report(self.virgin_all_edge_bitmap, 2)
        hash_main = self.read_values_report(self.virgin_main_path_bitmap, 8)
        hash_main = hash_main[0] if len(hash_main) > 0 else None
        hash_all = self.read_values_report(self.virgin_all_path_bitmap, 8)
        hash_all = hash_all[0] if len(hash_all) > 0 else None
                
        compare = time.time()
        # print("Path tracer: run=%.2f compare=%.2f" % (end-start, compare-end))

        return unique_main_edges, unique_all_edges, hash_main, hash_all, end-start, compare-end, status_code
        
    def read_values_report(self, report, element_size):
        
        if element_size == 2:
            format = "H"
        elif element_size == 4:
            format = "I"
        elif element_size == 8:
            format = "Q"
        else:
            assert False and "Invalid element size"
        
        values = []
        if os.path.exists(report):
            with open(report, "rb") as tmp:
                data = tmp.read(element_size)
                while data:
                    id = struct.unpack(format, data)[0]
                    values.append(id)
                    data = tmp.read(element_size)
        
        return values

    def initialize_bitmap(self, trace_bitmap):
        with open(trace_bitmap, "wb") as trace:
            for id in range(self.bitmap_size):
                trace.write(struct.pack('B', 0))
