#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <list>
#include "variable_queue.h"

using namespace rokid::queue;
using std::thread;
using std::mutex;
using std::list;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;

#define RAND_DATA_SIZE 128
#define MIN_WRITE_SIZE 7
#define QUEUE_MEM_SIZE 1024

typedef struct {
	int32_t size;
} WrittenRecord;

typedef struct {
	VariableQueue queue;
	list<WrittenRecord> written_rec;
	mutex locker;
	int16_t quit;
	int16_t result;
	uint32_t write_count;
	char rand_data[RAND_DATA_SIZE];
} TestInst;

static void write_to_queue(TestInst& inst) {
	uint32_t count = 100000;
	int sz;
	WrittenRecord rec;

	while (count) {
		sz = rand() % RAND_DATA_SIZE + MIN_WRITE_SIZE;
		if (sz > RAND_DATA_SIZE)
			sz = RAND_DATA_SIZE;
		inst.locker.lock();
		if (inst.queue.write(inst.rand_data, sz)) {
			rec.size = sz;
			inst.written_rec.push_back(rec);
			++inst.write_count;
			--count;
		}
		inst.locker.unlock();
	}
	inst.quit = 1;
}

static void read_from_queue(TestInst& inst) {
	std::string data;
	uint32_t sz;
	const void* rp;

	while (true) {
		inst.locker.lock();
		if (inst.written_rec.size()) {
			if (inst.queue.is_continuous()) {
				rp = inst.queue.peek(&sz);
			} else {
				sz = inst.queue.read(data);
				rp = data.data();
			}

			if (sz != inst.written_rec.front().size) {
				printf("read size %u not equal write size %u\n", sz, inst.written_rec.front().size);
				inst.result = -1;
			} else if (memcmp(inst.rand_data, rp, sz)) {
				printf("read data content not equal write data content, size = %u\n", sz);
				inst.result = -1;
			}
			if (inst.queue.is_continuous())
				inst.queue.erase();
			inst.written_rec.pop_front();
		} else if (inst.quit) {
			inst.locker.unlock();
			break;
		}
		inst.locker.unlock();
	}
}

int main(int argc, char** argv) {
	unsigned int sd = (unsigned int)duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
	srand(sd);

	void* mem = new char[QUEUE_MEM_SIZE];

	TestInst inst;
	inst.queue.create(mem, QUEUE_MEM_SIZE);
	inst.quit = 0;
	inst.result = 0;
	inst.write_count = 0;
	thread wr_th1([&inst]() { write_to_queue(inst); });
	thread rd_th1([&inst]() { read_from_queue(inst); });
	wr_th1.join();
	rd_th1.join();
	inst.queue.close();
	inst.written_rec.clear();
	printf("variable queue test result = %d, write count = %u\n",
			inst.result, inst.write_count);

	inst.queue.create(mem, QUEUE_MEM_SIZE, true);
	inst.quit = 0;
	inst.result = 0;
	inst.write_count = 0;
	thread wr_th2([&inst]() { write_to_queue(inst); });
	thread rd_th2([&inst]() { read_from_queue(inst); });
	wr_th2.join();
	rd_th2.join();
	inst.queue.close();
	printf("continuous variable queue test result = %d, write count = %u\n",
			inst.result, inst.write_count);
	return 0;
}
