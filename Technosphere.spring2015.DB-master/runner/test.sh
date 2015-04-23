log="test.log"
echo "Testing uniform" > $log
make test_uni 2>> $log
make test_uni 2>> $log
make test_uni 2>> $log
echo "Testing oldest" >> $log
make test_old 2>> $log
make test_old 2>> $log
make test_old 2>> $log
echo "Testing latest" >> $log
make test_lat 2>> $log
make test_lat 2>> $log
make test_lat 2>> $log
