TOPNAME = top
VSRC = vsrc/top.v
CSRC = csrc/main.cpp
BUILD_DIR = obj_dir

all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	verilator -Wall --cc $(VSRC) --exe $(CSRC) --top-module $(TOPNAME) --trace-fst --Mdir $(BUILD_DIR)
	make -j -C $(BUILD_DIR) -f V$(TOPNAME).mk V$(TOPNAME)
	./$(BUILD_DIR)/V$(TOPNAME)

clean:
	rm -rf $(BUILD_DIR) wave.fst

include ../Makefile
