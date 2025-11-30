CC=gcc
CFLAGS=-Wall -pthread
BIN=bin

all: dirs shop46 shm_mgr seller buyer obs disp10 obs10

dirs:
	mkdir -p $(BIN)

shop46:
	$(CC) src_46/shop46.c -o $(BIN)/shop46 $(CFLAGS)

shm_mgr:
	$(CC) src_78/shm_mgr.c -o $(BIN)/shm_mgr $(CFLAGS)

seller:
	$(CC) src_78/seller.c -o $(BIN)/seller $(CFLAGS)

buyer:
	$(CC) src_78/buyer.c -o $(BIN)/buyer $(CFLAGS)

obs:
	$(CC) src_9/obs.c -o $(BIN)/obs $(CFLAGS)

disp10:
	$(CC) src_10/disp10.c -o $(BIN)/disp10 $(CFLAGS)

obs10:
	$(CC) src_10/obs10.c -o $(BIN)/obs10 $(CFLAGS)

clean:
	rm -rf $(BIN)

