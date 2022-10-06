#!/usr/bin/python3 -u

import os
import sys
import executor
import signal
import argparse
import shutil
import limiter
import queue_manager


def kill_running_processes(verbose=False):
    limiter.SHUTDOWN = True
    P = limiter.RUNNING_PROCESSES[:]
    for p in P:
        # print("Terminating %s" % p.pid)
        if verbose:
            print("[SymFusion] Sending SIGINT")
        os.system("pkill -9 -P %s" % p.pid)
        p.send_signal(signal.SIGINT)
        p.send_signal(signal.SIGUSR2)
        try:
            p.wait(2)
            try:
                limiter.RUNNING_PROCESSES.remove(p)
            except:
                pass
        except:
            if verbose:
                print("[SymFusion] Sending SIGKILL")
            p.send_signal(signal.SIGKILL)
            try:
                limiter.RUNNING_PROCESSES.remove(p)
            except:
                pass
            # print("[SymFusion] Waiting for termination")
            # p.wait()
    if verbose:
        print("[SymFusion] Exiting")

def handler(signo, stackframe):
    print("[SymFusion] Aborting....")
    kill_running_processes(True)
    sys.exit(1)


def main():

    parser = argparse.ArgumentParser(
        description='SymFusion: hybrid concolic executor')

    # version
    parser.add_argument('--version', action='version',
                        version='%(prog)s pre-{\\alpha}^{\infty}')

    # optional args
    parser.add_argument(
        '-d', '--debug', choices=['output', 'gdb', 'tracer'], help='enable debug mode; a single input will be analyzed')
    parser.add_argument(
        '-a', '--afl', help='path to afl workdir (it enables AFL mode); AFL_PATH must be set.')
    parser.add_argument(
        '-t', '--timeout', type=int, help='maximum running time for each input (secs)')
    parser.add_argument(
        '-k', '--keep-run-dirs', action='store_false', help='keep run directories')
    parser.add_argument(
        '-c', '--cache', help='path to cache directory')
    parser.add_argument(
        '-m', '--hybrid-mode', action='store_false', help='Run in hybrid mode')
    parser.add_argument(
        '-g', '--generate-hybrid-conf', help='Only generate hybrid conf at the given path')
    parser.add_argument(
        '-f', '--fork-server', action='store_true', help='Run using a fork server')
    parser.add_argument(
        '-q', '--queue-mode', choices=['symfusion', 'qsym', 'hash'], help='The type of queue handling used when picking inputs')

    # required args
    parser.add_argument(
        '-i', '--input', help='path to the initial seed (or seed directory)', required=True)
    parser.add_argument(
        '-o', '--output', help='output directory', required=True)

    # positional args
    parser.add_argument('binary', metavar='<binary>',
                        type=str, help='path to the binary to run')
    parser.add_argument('args', metavar='<args>', type=str, help='args for the binary to run',
                        nargs='*')  # argparse.REMAINDER

    args = parser.parse_args()

    binary = args.binary
    if not os.path.exists(binary):
        sys.exit('ERROR: binary does not exist.')

    input = args.input
    if not os.path.exists(input):
        sys.exit('ERROR: input does not exist.')

    PLACEHOLDER = ".symfusion"
    output_dir = args.output
    if args.generate_hybrid_conf is None:
        if os.path.exists(output_dir):
            if os.path.exists(output_dir + '/' + PLACEHOLDER):
                shutil.rmtree(output_dir)
            else:
                sys.exit("Unsafe to remove %s. Do it manually." % output_dir)
        if not os.path.exists(output_dir):
            os.system("mkdir -p " + output_dir)
            if not os.path.exists(output_dir):
                sys.exit('ERROR: cannot create output directory.')
            os.system("touch " + output_dir + '/' + PLACEHOLDER)

    if args.afl and not os.path.exists(args.afl):
        sys.exit('ERROR: AFL wordir does not exist.')
    afl = args.afl

    binary_args = args.args
    debug = args.debug
    cache_dir = args.cache
    if cache_dir:
        cache_dir = os.path.abspath(cache_dir)

    timeout = args.timeout
    keep_run_dirs = args.keep_run_dirs
    if keep_run_dirs is None:
        keep_run_dirs = True
        
    if args.debug == "gdb" and args.fork_server:
        print("Cannot debug with GDB when using forkserver")
        sys.exit(1)
        
    queue_mode = queue_manager.QueueManager.SYMFUSION_MODE
    if args.queue_mode == "qsym":
        queue_mode = queue_manager.QueueManager.QSYM_MODE
    elif args.queue_mode == "hash":
        queue_mode = queue_manager.QueueManager.HASH_MODE
    elif args.queue_mode == "symfusion":
        queue_mode = queue_manager.QueueManager.SYMFUSION_MODE
        
    signal.signal(signal.SIGINT, handler)

    symfusion = executor.Executor(
        binary, input, output_dir, binary_args, debug, afl,
        timeout=timeout, keep_run_dirs=keep_run_dirs, cache_dir=cache_dir,
        hybrid_mode=args.hybrid_mode, generate_hybrid_conf=args.generate_hybrid_conf,
        forkserver=args.fork_server, queue_mode=queue_mode)
    symfusion.run()
    kill_running_processes()


if __name__ == "__main__":
    main()
