#include <sys/time.h>

#include <cstring>
#include <vector>
#include <iostream>
#include <fstream>

#include <yaml-cpp/yaml.h>

#include "database.h"

#ifndef clock_gettime
#  ifdef __APPLE__
#    include <mach/mach_time.h>
#    define ORWL_NANO          (+1.0E-9   )
#    define ORWL_GIGA UINT64_C (1000000000)
#    define CLOCK_MONOTONIC    0
int clock_gettime(int flag, struct timespec *tspec) {
	(void)flag;
	static double orwl_timebase = 0.0;

	mach_timebase_info_data_t tb = { 0 };
	mach_timebase_info(&tb);
	orwl_timebase = tb.numer;
	orwl_timebase /= tb.denom;
	double diff = mach_absolute_time() * orwl_timebase;
	tspec->tv_sec = diff * ORWL_NANO;
	tspec->tv_nsec = diff - (tspec->tv_sec * ORWL_GIGA);
	return 0;
}
#  endif /* __APPLE__ */
#endif /* clock_gettime */

int main(int argc, char *argv[]) {
	std::string def_so_name = "./libmydb.so";
	std::string def_db_name = "./mydbpath";
	std::string def_wl_name = "../workloads/workload.uni";

	if (argc > 1) def_wl_name = std::string(argv[1]);
	if (argc > 2) def_so_name = std::string(argv[2]);

	Database *db = new Database(def_so_name.c_str(), def_db_name.c_str());
	YAML::Node workload = YAML::LoadFile((def_wl_name + ".in").c_str());
	std::ofstream out ((def_wl_name + ".out.yours").c_str());
	struct timespec t1, t2; memset(&t1, 0, sizeof(t1)); memset(&t2, 0, sizeof(t2));
	uint64_t time = 0;
	for (YAML::const_iterator it = workload.begin(); it != workload.end(); ++it) {
		auto op = it->as<std::vector<std::string>>();
		int retval = 0;
		if (op[0] == std::string("put")) {
			clock_gettime(CLOCK_MONOTONIC, &t1);
			retval = db->insert(op[1], op[2]);
			clock_gettime(CLOCK_MONOTONIC, &t2);
			time += (t2.tv_sec - t1.tv_sec) * 1e9 + (t2.tv_nsec - t1.tv_nsec);
		} else if (op[0] == std::string("get")) {
			char *val = NULL;
			size_t val_size = 0;
			clock_gettime(CLOCK_MONOTONIC, &t1);
			retval = db->select(op[1], &val, &val_size);
			clock_gettime(CLOCK_MONOTONIC, &t2);
			time += (t2.tv_sec - t1.tv_sec) * 1e9 + (t2.tv_nsec - t1.tv_nsec);
			out.write(val, val_size) << "\n";
		} else if (op[0] == std::string("del")) {
			char *val = NULL;
			size_t val_size = 0;
			clock_gettime(CLOCK_MONOTONIC, &t1);
			retval = db->del(op[1]);
			clock_gettime(CLOCK_MONOTONIC, &t2);
			time += (t2.tv_sec - t1.tv_sec) * 1e9 + (t2.tv_nsec - t1.tv_nsec);
			db->select(op[1], &val, &val_size);
			if (!val)
				out << "delete is ok\n";
		} else {
			std::cout << "bad op\n";
		}
	}
	std::cerr << "Overall lib time: " << ((double )time / 1e9) << "\n";
	return 0;
}
