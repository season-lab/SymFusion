all:
	@echo "Targets: rebuild"

rebuild:
	cd symcc-hybrid/build && ninja
	cd symqemu-hybrid && make