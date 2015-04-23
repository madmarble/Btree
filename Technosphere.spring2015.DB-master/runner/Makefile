all:
	g++ main.cpp database.cpp -lyaml-cpp -ldl -o test_speed --std=c++11 -lpthread -O2\
		-I/usr/local/Cellar/yaml-cpp/0.5.1/include/ \
		-I/usr/local/Cellar/boost/1.56.0/include/ \
		-L/usr/local/Cellar/yaml-cpp/0.5.1/lib/ \
		-L/usr/local/Cellar/boost/1.56.0/lib/

test_uni:
	rm -rf ./mydbpath
	./test_speed ../workloads/workload.uni
#	diff ../workload.uni.out ../workload.uni.out.yours >> diffs

test_old:
	rm -rf ./mydbpath
	./test_speed ../workloads/workload.old
#	diff ../workload.old.out ../workload.old.out.yours >> diffs

test_lat:
	rm -rf ./mydbpath
	./test_speed ../workloads/workload.lat
#	diff ../workload.lat.out ../workload.lat.out.yours >> diffs

test_all:
	./test.sh
