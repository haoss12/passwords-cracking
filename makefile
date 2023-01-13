make: md5_test1.cpp
	g++ -o md5 md5_test1.cpp -lcrypto -lpthread -std=c++2a -Wall -pedantic
