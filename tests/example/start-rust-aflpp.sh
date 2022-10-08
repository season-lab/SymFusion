#!/bin/bash

/afl/afl-clang-fast example.c -o example.afl
../../symcc-hybrid/build/symcc -o example.symfusion example.c

export OUTPUT=`pwd`/out
rm -rf ${OUTPUT}
mkdir ${OUTPUT}

export HYBRID_CONF_FILE=$OUTPUT/hybrid.conf
export LD_BIND_NOW=1
export SYMFUSION_HYBRID=1
export WRAPPER=`pwd`/../../symqemu-hybrid/x86_64-linux-user/symqemu-x86_64
export SYMCC_ENABLE_LINEARIZATION=1

export SEEDS=`pwd`/inputs

../../runner/symfusion.py -g ${HYBRID_CONF_FILE} -i ${SEEDS} -o ${OUTPUT}/concolic -- ./example.symfusion @@
rm -rf ${OUTPUT}/concolic

/afl/afl-fuzz -M afl-master -t 5000 -m 100M -i ${SEEDS} -o ${OUTPUT} -- ./example.afl @@ >/dev/null 2>&1 &

FUZZER_PID=$!
while ps -p $FUZZER_PID > /dev/null 2>&1 && \
    [[ ! -f "${OUTPUT}/afl-master/fuzzer_stats" ]]; do
    echo "Waiting fuzzer to start..." && sleep 1
done

while ps -p $FUZZER_PID > /dev/null 2>&1 && \
    [[ ! -f "${OUTPUT}/afl-master/fuzz_bitmap" ]]; do
    echo "Waiting fuzzer to create bitmap..." && sleep 1
done

../../symcc-hybrid/build/symcc_fuzzing_helper -a afl-master -o ${OUTPUT} -n concolic -- ${WRAPPER} ./example.symfusion @@