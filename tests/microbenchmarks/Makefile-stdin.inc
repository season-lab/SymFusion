ifndef DIRECTORY
DIRECTORY=${BINARY}
endif

clean:
	mkdir out${SYMFUSION_FORKSERVER} || echo
	rm -rf out${SYMFUSION_FORKSERVER}/* || echo
	touch out${SYMFUSION_FORKSERVER}/.symfusion

symfusion: clean
	LD_LIBRARY_PATH=`pwd` ../../../runner/symfusion.py -i ${BINARY_SEED} -o out -d output -- ./${BINARY}.symfusion ${BINARY_ARGS}

symcc:
	rm -rf out_symcc || echo
	mkdir out_symcc
	LD_LIBRARY_PATH=`pwd` SYMCC_OUTPUT_DIR=./out_symcc ../../../utils/time.sh ./${BINARY}.symcc ${BINARY_ARGS} < ${BINARY_SEED}

symqemu:
	rm -rf out_symqemu || echo
	mkdir out_symqemu
	LD_LIBRARY_PATH=`pwd` SYMCC_OUTPUT_DIR=./out_symqemu ../../../utils/time.sh ../../../original/symqemu/x86_64-linux-user/symqemu-x86_64 ${BINARY}.symqemu ${BINARY_ARGS} < ${BINARY_SEED}