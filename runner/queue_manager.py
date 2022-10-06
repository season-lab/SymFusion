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
import random
import ctypes
import resource
import collections
import struct

from path_tracer import PathTracer

from limiter import setlimits, RUNNING_PROCESSES

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

class Queue(object):
    def __init__(self, name, only_unique_ids=True):
        self.name = name
        self.ids = {}           # id -> (testcase, was_processed)
        self.testcase_ids = {}  # testcase -> list of ids
        self.count_ids = {}     # testcase -> number of ids to process for the testcase
        self.only_unique_ids = only_unique_ids

    def add(self, testcase, ids, imported=False):
        assert testcase not in self.count_ids
        inserted = False
        for id in ids:
            if id not in self.ids:
                self.ids[id] = {}
                self.ids[id][testcase] = False
                inserted = True
            elif not self.only_unique_ids:
                self.ids[id][testcase] = False
        if inserted:     
            self.count_ids[testcase] = len(ids) + (1 if imported else 0)
            self.testcase_ids[testcase] = ids

    def testcase_with_highest_count(self):
        if len(self.count_ids) == 0:
            return None, 0
        max_key = max(self.count_ids, key=self.count_ids.get)
        if self.count_ids[max_key] > 0:
            return max_key, self.count_ids[max_key]
        else:
            return None, 0

    def mark_testcase_as_processed(self, testcase):
        if testcase in self.count_ids:
            for id in self.testcase_ids[testcase]:
                self.ids[id][testcase] = True
            self.count_ids[testcase] = 0
        
    def get_testcases_for_id(self, id):
        if id in self.ids:
            return self.ids[id].keys()
        else:
            return None
        
    def size(self):
        return len(self.count_ids)
    
    def count_unprocessed(self):
        count = 0
        for testcase in self.count_ids:
            if self.count_ids[testcase] > 0:
                count += 1
        return count        
        

class QueueManager(object):
    HASH_MODE = 1
    QSYM_MODE = 2
    SYMFUSION_MODE = 3

    def __init__(self, root_dir, binary, binary_args, hybrid_mode=False, hybrid_conf=None, afl_dir=None, mode=HASH_MODE):
        self.mode = mode
        self.root_dir = root_dir
        self.queue_dir = "%s/queue" % root_dir
        os.system("mkdir %s" % self.queue_dir)
        
        self.afl_dir = afl_dir
        self.afl_min_id = -1
        
        if mode == QueueManager.SYMFUSION_MODE:
            self.tracer = PathTracer(binary, binary_args, root_dir, hybrid_mode=hybrid_mode, hybrid_conf=hybrid_conf)
            self.queue_main_edges = Queue("Edges (application code)", only_unique_ids=False)
            self.queue_all_edges = Queue("Edges (all code)", only_unique_ids=False)
            self.queue_main_path = Queue("Paths (application code)")
            self.queue_all_path = Queue("Paths (all code)")
        
        elif mode == QueueManager.HASH_MODE:
            self.curr_gen_dir = "%s/curr" % root_dir
            os.system("mkdir %s" % self.curr_gen_dir)
            self.next_gen_dir = "%s/next" % root_dir
            os.system("mkdir %s" % self.next_gen_dir)
            self.hash_generated_inputs = set()
            self.hash_processed_inputs = set()
            
        elif mode == QueueManager.QSYM_MODE:
            afl_bin_dir = os.environ['AFL_PATH']
            assert afl_bin_dir is not None
            self.afl_tracer = afl_bin_dir + "/afl-showmap"
            self.afl_binary_args = binary_args
            
            if False:
                binary = os.path.basename(binary).replace(".symfusion", "")
                binary = glob.glob("/home/symbolic/qemu-hybrid-test/tests/*/%s" % binary + ".symqemu")[0]
                binary = binary.replace(".symqemu", ".symfusion")
            
            if os.path.exists(binary.replace(".symfusion", ".afl")) and False:
                self.afl_tracer_qemu_mode = False
                self.afl_binary = binary.replace(".symfusion", ".afl") # FIXME
            else:
                assert os.path.exists(binary.replace(".symfusion", ".symqemu"))
                self.afl_tracer_qemu_mode = True
                self.afl_binary = binary.replace(".symfusion", ".symqemu") # FIXME
            assert os.path.exists(self.afl_tracer)
            self.afl_tracer_bitmap = root_dir + "/afl_bitmap"
            self.processed_inputs = set()
            self.inputs = {}
            
            self.afl_tracer_persistent = True
            
            if self.afl_tracer_persistent:
                assert self.afl_tracer_qemu_mode
                self.afl_pipe_read = self.root_dir + "/afl_pipe_read"
                self.afl_pipe_write = self.root_dir + "/afl_pipe_write"
                os.mkfifo(self.afl_pipe_read)
                os.mkfifo(self.afl_pipe_write)
                os.system("mkdir %s" % self.root_dir + "/tracer")
                self.afl_tracer_output_dir = self.root_dir + "/tracer/out"
                self.afl_tracer_input_dir = self.root_dir + "/tracer/in"
                
                proc, start = self.start_afl_tracer(input_dir=self.afl_tracer_input_dir, output_dir=self.afl_tracer_output_dir)
        
        self.tick_count = 0

    def add_from_dir(self, run_dir, source_testcase, min_id=None, sync=False):
        
        runner_time = 0
        comparer_time = 0
        duplicate = 0
        unique = 0
        
        if self.mode in [QueueManager.SYMFUSION_MODE, QueueManager.HASH_MODE]:
            for testcase in sorted(glob.glob(run_dir + "/*")):
                if testcase.endswith(".log"):
                    continue
                if sync:
                    source_testcase = "%s" % os.path.basename(testcase).split(",")[0].split("id:")[1]
                if os.path.basename(testcase) == os.path.basename(source_testcase):
                    continue
                if os.path.basename(testcase) == "input_file":
                    continue
                if min_id is not None: 
                    testcase_id = int(os.path.basename(testcase)[3:9])
                    if testcase_id <= min_id:
                        continue
                    min_id = testcase_id
                    # print("ID: %s" % testcase_id)
                
                rt, ct, dup = self.add(testcase, source_testcase, sync)
                runner_time += rt
                comparer_time += ct
                if dup:
                    duplicate += 1
                else:
                    unique += 1
        
        elif self.mode == QueueManager.QSYM_MODE:
            
            input_dir = run_dir
            if sync or self.afl_tracer_persistent:
                if not self.afl_tracer_persistent:
                    input_dir_tmp = tempfile.TemporaryDirectory()
                    input_dir = input_dir_tmp.name
                else:
                    os.system("rm -rf %s" % self.afl_tracer_input_dir)
                    os.system("mkdir %s" % self.afl_tracer_input_dir)
                    input_dir = self.afl_tracer_input_dir
                for testcase in sorted(glob.glob(run_dir + "/*")):
                    if min_id is not None: 
                        testcase_id = int(os.path.basename(testcase)[3:9])
                        if testcase_id < min_id:
                            continue
                        min_id = testcase_id
                    os.system("cp %s %s/" % (testcase, input_dir))
            
            if not self.afl_tracer_persistent:
                tmpdir = tempfile.TemporaryDirectory() 
                output_dir = tmpdir.name
            else:
                os.system("rm -rf %s" % self.afl_tracer_output_dir)
                os.system("mkdir %s" % self.afl_tracer_output_dir)
                output_dir = self.afl_tracer_output_dir
                
            if not self.afl_tracer_persistent:
                proc, start = self.start_afl_tracer(input_dir=input_dir, output_dir=output_dir)

            else:
                start = time.time()
                
                self.pipe_write.write(bytes([23, 23, 23, 23]))
                self.pipe_write.flush()
                
                data = self.pipe_read.read(4)
                
            if not self.afl_tracer_persistent:
                proc.wait()
                RUNNING_PROCESSES.remove(proc)
                
            runner_time = time.time() - start

            for f in sorted(glob.glob(output_dir + "/*")):
                start = time.time()
                if not self.afl_tracer_persistent:
                    interesting, new_coverage = self.is_interesting_bitmap(f)
                else:
                    with open(f, "rb") as fp:
                        count_interesting = struct.unpack("I", fp.read(4))[0]
                        count_new_coverage = struct.unpack("I", fp.read(4))[0]
                        interesting = count_interesting > 0
                        new_coverage = count_new_coverage > 0

                comparer_time += time.time() - start
                if interesting:
                    unique += 1
                else:
                    duplicate += 1
                    
                if interesting:
                    print("Testcase %s: interesting=%s new_cov=%s" % (os.path.basename(f), interesting, new_coverage))
                    if sync:
                        source_testcase = os.path.basename(f)[:9]
                    else:
                        source_testcase = os.path.basename(source_testcase)
                    name = "id:%06d,src:%s" % (self.tick(), source_testcase if "id:" not in source_testcase else source_testcase[:len("id:......")])
                    if sync:
                        name += ",sync:afl"
                    if new_coverage:
                        name += ",+cov"
                    new_testcase = "%s/%s" % (self.queue_dir, name)
                    # print(new_testcase)
                    os.system("cp %s/%s %s" % (input_dir, os.path.basename(f), new_testcase))
                    self.inputs[name] = get_score(f)
                
        print("Summary of testcases: duplicated=%d unique=%d tracer_time=%.2f" % (duplicate, unique, runner_time))
        return runner_time, comparer_time, min_id
    
    def start_afl_tracer(self, input_dir, output_dir):
        cmd = [self.afl_tracer,
            "-t",
            str(3000),
            "-m", "1G",
            "-b" # binary mode
        ]

        if self.afl_tracer_qemu_mode:
            cmd += ['-Q']

        cmd += ["-o",
            output_dir,
            '-i',
            input_dir,
            "--",
        ] + [ self.afl_binary ] + self.afl_binary_args

        # print(cmd)
        env = os.environ.copy()
        del env['AFL_PATH']

        if self.afl_tracer_qemu_mode:
            del env['AFL_MAP_SIZE']

        if self.afl_tracer_persistent:
            env['SYMFUSION_TRACER_PIPE_READ'] = self.afl_pipe_write
            env['SYMFUSION_TRACER_PIPE_WRITE'] = self.afl_pipe_read
            # env['AFL_INST_LIBS'] = "1"
        
        start = time.time()
        proc = subprocess.Popen(cmd, stdin=None, 
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, 
                                env=env)
        RUNNING_PROCESSES.append(proc)
        
        if self.afl_tracer_persistent:
            self.pipe_write = open(self.afl_pipe_write, 'wb')
            self.pipe_read = open(self.afl_pipe_read, 'rb')
            
            data = self.pipe_read.read(4)
        
        return proc, start
    
    def is_interesting_bitmap(self, bitmap):
        if not os.path.exists(self.afl_tracer_bitmap):
            os.system("cp %s %s" % (bitmap, self.afl_tracer_bitmap))
            return True, True

        cmd = [
            SCRIPT_DIR + '/../utils/comparer',
            bitmap,
            self.afl_tracer_bitmap,
            str(os.path.getsize(bitmap))
        ]
        # print(cmd)

        with open(os.devnull, "wb") as devnull:
            proc = subprocess.Popen(cmd, stdin=None, 
                                    stdout=devnull, stderr=devnull
                                    )
            proc.wait()
            if proc.returncode == 1:
                return True, False
            if proc.returncode == 2:
                return True, True
            elif proc.returncode == 0:
                return False, False
            else:
                print("Error while merging bitmap %s [error code %d]" % (bitmap, proc.returncode))
                return False, False

        print("Error while merging bitmaps")

    def add(self, testcase, source_testcase, sync=False):
        
        if self.mode == QueueManager.QSYM_MODE:
            print("Use add_from_dir()")
            sys.exit(1)
        
        duplicate = False
        if self.mode == QueueManager.SYMFUSION_MODE:
            new_main_edge_ids, new_all_edge_ids, hash_main, hash_all, runner_time, comparer_time, status_code = self.tracer.run(
                testcase)
            
            # return runner_time, comparer_time, True
            
            if hash_main is None or hash_all is None: # likely a crash
                print("Crashing input in the path tracer: status_code=%s input=%s" % (status_code, os.path.basename(testcase)))
                return runner_time, comparer_time, True

            is_unique_path_main = self.queue_main_path.get_testcases_for_id(hash_main) is None
            is_unique_path_all = self.queue_all_path.get_testcases_for_id(hash_all) is None
            if len(new_main_edge_ids) == 0 and len(new_all_edge_ids) == 0 and not is_unique_path_main and not is_unique_path_all:
                duplicate = True

            if not duplicate:            
                print("Testcase %s%s: new_edges=[app=%d, all=%d], unique_path=[app=%s, all=%s]" % (
                    os.path.basename(testcase), 
                    " [%x, %x]" % (hash_main, hash_all),
                    len(new_main_edge_ids), len(new_all_edge_ids), is_unique_path_main,is_unique_path_all))

            if duplicate:
                return runner_time, comparer_time, duplicate

            debug_queue = False
            if debug_queue:
                if len(new_main_edge_ids) > 0:
                    assert len(new_all_edge_ids) > 0
                    assert is_unique_path_main
                    assert is_unique_path_all
                if len(new_all_edge_ids) > 0:
                    assert is_unique_path_all
                if is_unique_path_main:
                    if not is_unique_path_all:
                        print("Testcase generate a unique path for the application code but not for the entire execution")
                        if hash is not None:
                            print("Testcase with same hash as testcase: %s" % self.queue_all_path.get_testcases_for_id(hash_all)) 
                    assert is_unique_path_all
        
        elif self.mode == QueueManager.HASH_MODE:
            start = time.time()
            hash = subprocess.check_output("sha256sum %s | cut -f1 -d' '" % testcase, shell=True)
            hash = hash.decode('ascii').rstrip("\n")
            if hash in self.hash_generated_inputs:
                duplicate = True
            runner_time = time.time() - start
            comparer_time = 0
            
            if duplicate:
                return runner_time, comparer_time, duplicate
            
            self.hash_generated_inputs.add(hash)
            os.system("cp %s %s" % (testcase, self.next_gen_dir + "/" + hash))
            print("Testcase %s: hash=%s" % (os.path.basename(testcase), hash))
        
        source_testcase = os.path.basename(source_testcase)
        name = "id:%06d,src:%s" % (self.tick(
        ), source_testcase if "id:" not in source_testcase else source_testcase[:len("id:......")])
        if sync:
            name += ",sync:afl"
        new_testcase = "%s/%s" % (self.queue_dir, name)
        os.system("cp %s %s" % (testcase, new_testcase))

        if self.mode == QueueManager.SYMFUSION_MODE:
            if len(new_main_edge_ids) > 0:
                self.queue_main_edges.add(new_testcase, new_main_edge_ids, imported=sync)
            if len(new_all_edge_ids) > 0:
                self.queue_all_edges.add(new_testcase, new_all_edge_ids, imported=sync)
            self.queue_main_path.add(new_testcase, [ hash_main ], imported=sync)
            self.queue_all_path.add(new_testcase, [ hash_all ], imported=sync)
        
        return runner_time, comparer_time, duplicate
    
    def import_from_afl(self):
        if self.afl_dir is None:
            return 0, 0

        print("\nImporting from AFL...")
        runner_time, comparer_time, self.afl_min_id = self.add_from_dir(self.afl_dir + "/queue", None, self.afl_min_id, True)

        return runner_time, comparer_time

    def pick_testcase(self):
        tracer_time, comparer_time = self.import_from_afl()
        
        if self.mode == QueueManager.SYMFUSION_MODE:
            queues = [self.queue_main_edges,
                    self.queue_all_edges, 
                    self.queue_main_path,
                    self.queue_all_path]
            for queue in queues:
                testcase, score = queue.testcase_with_highest_count()
                if testcase:
                    for q in queues:
                        q.mark_testcase_as_processed(testcase)
                    print("\nPicking input from queue %s: %s [score=%d, count=%d, waiting=%d]" %
                        (queue.name, os.path.basename(testcase), score, queue.size(), queue.count_unprocessed()))
                    return testcase, tracer_time, comparer_time
                
        elif self.mode == QueueManager.HASH_MODE:
            
            testcases = sorted(glob.glob(self.curr_gen_dir + "/*"))
            if len(testcases) == 0:
                print("\nNew generation...")
                os.system("rmdir %s" % self.curr_gen_dir)
                os.system("mv %s %s" % (self.next_gen_dir, self.curr_gen_dir))
                os.system("mkdir %s" % self.next_gen_dir)
                testcases = sorted(glob.glob(self.curr_gen_dir + "/*"))
            
            for testcase in testcases:
                new_testcase = self.root_dir + "/current_gen_input.dat"
                os.system("cp %s %s" % (testcase, new_testcase))
                os.system("rm %s" % testcase)
                hash = os.path.basename(testcase)
                self.hash_processed_inputs.add(hash)
                print("\nPicking input from queue: %s (waiting=%d)" % (hash, len(testcases)-1))
                return new_testcase, tracer_time, comparer_time
            
        elif self.mode == QueueManager.QSYM_MODE:
            start = time.time()
            if len(self.inputs) > 0:
                testcase = keywithmaxval(self.inputs)
                del self.inputs[testcase]
                self.processed_inputs.add(testcase)
                end = time.time()
                print("\nPicking input from queue: %s (waiting=%d, time=%.2f)" % (testcase, len(self.inputs), end-start))
                return self.queue_dir + "/" + testcase, tracer_time, comparer_time
        
        return None, 0, 0

    def tick(self):
        self.tick_count += 1
        return self.tick_count - 1


def get_score(testcase):
    # New coverage is the best
    score1 = testcase.endswith("+cov")
    # NOTE: seed files are not marked with "+cov"
    # even though it contains new coverage
    score2 = "seed" in testcase
    # Smaller size is better
    try:
        score3 = -os.path.getsize(testcase)
    except:
        score3 = -10000 # file has been renamed by AFL
    # Since name contains id, so later generated one will be chosen earlier
    score4 = testcase
    return (score1, score2, score3, score4)

def testcase_compare(a, b):
    a_score = get_score(a)
    b_score = get_score(b)
    return 1 if a_score > b_score else -1

def keywithmaxval(d):
     """ a) create a list of the dict's keys and values; 
         b) return the key with the max value"""  
     v=list(d.values())
     k=list(d.keys())
     return k[v.index(max(v))]