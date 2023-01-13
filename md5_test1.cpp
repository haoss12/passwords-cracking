#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <openssl/evp.h>
#include <chrono>
#include "md5_app.hh"

int main(int argc, char** argv)
{

	App *a;
	try
	{
		if(argc == 3)
		{
			const char *s1 = argv[1];
			const char *s2 = argv[2];
			std::string data_path (s1);
			std::string dict_path (s2);
			a = new App(data_path, dict_path);
		}
		else
		{
			a = new App;
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return -1;
	}
	//a->print_data();
	a->run();

	return 0;

}
