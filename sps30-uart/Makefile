# See user_config.inc for build customizations
-include user_config.inc
include default_config.inc

.PHONY: all clean

all: run_sps30

run_sps30: clean
	$(CC) $(CFLAGS) -o $@ ${sps30_uart_sources} ${uart_sources} ${sps30_uart_dir}/run_sps30.c

clean:
	$(RM) run_sps30
