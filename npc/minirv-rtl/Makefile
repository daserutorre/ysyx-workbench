TOPNAME = top
VSRC = $(shell find vsrc -name "*.v")
CSRC = csrc/main.cpp csrc/dpi.cpp csrc/minirvEMU.cpp
BUILD_DIR = obj_dir

all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	verilator -Wall -Wno-DECLFILENAME -Wno-UNUSEDSIGNAL -Wno-PINCONNECTEMPTY --cc $(VSRC) --exe $(CSRC) --top-module $(TOPNAME) --trace-fst --Mdir $(BUILD_DIR)
	make -j -C $(BUILD_DIR) -f V$(TOPNAME).mk V$(TOPNAME)
	./$(BUILD_DIR)/V$(TOPNAME)

$(BUILD_DIR)/V$(TOPNAME): $(VSRC) $(CSRC)
	verilator -Wall -Wno-DECLFILENAME -Wno-UNUSEDSIGNAL -Wno-PINCONNECTEMPTY --cc $(VSRC) --exe $(CSRC) --top-module $(TOPNAME) --trace-fst --Mdir $(BUILD_DIR)
	make -j -C $(BUILD_DIR) -f V$(TOPNAME).mk V$(TOPNAME)

# Used by AM's scripts/platform/npc.mk 'run' rule: builds (if needed) and
# runs the simulator on a given compiled program image, without triggering
# the git-tracking commit or a forced rebuild every time.
run: $(BUILD_DIR)/V$(TOPNAME)
	./$(BUILD_DIR)/V$(TOPNAME) $(BIN)

clean:
	rm -rf $(BUILD_DIR) wave.fst

.PHONY: all sim run clean

include ../../Makefile
