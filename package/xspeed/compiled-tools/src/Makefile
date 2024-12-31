OUT_DIR := $(TOP_DIR)sbin/
FILES := $(wildcard ./*)
FILES := $(filter-out %/, $(FILES))

all: clean $(OUT_DIR) compiled-tools

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

compiled-tools:
	cp $(FILES) $(OUT_DIR)
	rm $(OUT_DIR)/Makefile

clean:
