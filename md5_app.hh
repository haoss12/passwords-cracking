#ifndef MD5_CLASSES

#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <stdexcept>
#include <csignal>
#include "md5_utils.hh"

#define MAX_THREADS = 8;

class App
{
public:

    App();
    App(const std::string userdata_path, const std::string dictionary_path);
    ~App() = default;
    void run();

private:

    std::vector<std::string> dictionary;
    std::mutex _mutex;
    std::vector<userData> data;
    std::condition_variable cv_to_consumer;
    std::condition_variable cv_to_producers;
    std::vector<userData> cracked;
    bool stop_consumer;
    bool stop_stdin;
    bool reset_producers;
    inline static int cracked_count;
    std::vector<std::thread> thread_pool;

    void print_data() const;
    void print_cracked() const;
    static void signal_handler (int signum);
    void stdin_handler();
    void change_data(const std::string &pathname);
    void hash_and_find(const std::string &word);

    void producer1();   //no change, first capital, caps lock
    void producer2();   //number at front and back
    void numbers_both_sides();
    void two_words(const int start, const int jump);
    void consumer();

    //methods below not used right now, TODO: implement new data handling in it
    void no_change();
    void number_at_front();
    void number_at_back();
    void first_upper();
    void all_upper();

};

App::App(): stop_consumer(false), stop_stdin(false), reset_producers(false)
{
    std::string userdata_path = "example_data";
    std::string dictionary_path = "example_dict";
    // std::cout << "Enter name of file with users data: ";
    // std::cin >> userdata_path;
    // std::cout << "Enter name of file with dictionary: ";
    // std::cin >> dictionary_path;

    std::ifstream dictionary_file(dictionary_path);
    std::string word;

    if(!dictionary_file.is_open())
        throw std::invalid_argument("Dictionary file does not exist or cannot be open!");

    while(dictionary_file >> word)
		dictionary.push_back(word);

    std::ifstream users_file(userdata_path);
	int temp_id; 
	std::string temp_hash, temp_email, temp_nickname;
    userData temp;
	temp.isCracked = false;

    if(!users_file.is_open())
    {
        throw std::invalid_argument("Users file does not exist or cannot be open!");
    }

	while(users_file >> temp_id >> temp_hash >> temp_email)
	{
		std::getline(users_file, temp_nickname);
		ftrim(temp_nickname);
		temp.id = temp_id;
		temp.hash = temp_hash;
		temp.email = temp_email;
		temp.nickname = temp_nickname;
		data.push_back(temp);
    }


    std::signal(SIGHUP, App::signal_handler);

}

App::App(const std::string userdata_path, const std::string dictionary_path): stop_consumer(false), stop_stdin(false), reset_producers(false)
{

    std::ifstream dictionary_file(dictionary_path);
    std::string word;

    std::cout << "trying to read: " << dictionary_path << " file\n";

    if(!dictionary_file.is_open())
        throw std::invalid_argument("Dictionary file does not exist or cannot be open!");

    while(dictionary_file >> word)
		dictionary.push_back(word);

    std::ifstream users_file(userdata_path);
	int temp_id;
	std::string temp_hash, temp_email, temp_nickname;
    userData temp;
	temp.isCracked = false;

    std::cout << "trying to read: " << userdata_path << " file\n";

    if(!users_file.is_open())
    {
        throw std::invalid_argument("Users file does not exist or cannot be open!");
    }

	while(users_file >> temp_id >> temp_hash >> temp_email)
	{
		std::getline(users_file, temp_nickname);
		ftrim(temp_nickname);
		temp.id = temp_id;
		temp.hash = temp_hash;
		temp.email = temp_email;
		temp.nickname = temp_nickname;
		data.push_back(temp);
    }


    std::signal(SIGHUP, App::signal_handler);

}

void App::signal_handler (int signum)
{
    std::fflush(stdout);
    std::cout << "Number of passwords cracked: " << cracked_count << "\n";
}

void App::print_data() const
{
    for(auto const var : data)
    {
        std::cout << var << '\n';
    }
}

void App::print_cracked() const
{
    for(auto const var : cracked)
    {
        std::cout << var << '\n';
    }
}

void App::change_data(const std::string &pathname)
{
    std::ifstream users_file(pathname);

    if(!users_file.is_open())
    {
        throw std::invalid_argument("Users file does not exist or cannot be open!");
    }

    std::unique_lock<std::mutex> lock(_mutex);
    reset_producers = true;

    lock.unlock();

    int temp_id;
	std::string temp_hash, temp_email, temp_nickname;
    userData temp;
	temp.isCracked = false;

    std::vector<userData> new_data;


    std::cout << "changing data" << "\n";


	while(users_file >> temp_id >> temp_hash >> temp_email)
	{
		std::getline(users_file, temp_nickname);
		ftrim(temp_nickname);
		temp.id = temp_id;
		temp.hash = temp_hash;
		temp.email = temp_email;
		temp.nickname = temp_nickname;
		new_data.push_back(temp);
    }

    lock.lock();

    data = new_data;
    cracked.clear();
    cracked_count = 0;
    reset_producers = false;
    cv_to_producers.notify_all();

    lock.unlock();

}

void App::stdin_handler()
{
    std::string pathname;
    while (stop_stdin != true)
    {
        std::cin >> pathname;
        try
	    {
		    change_data(pathname);
            std::unique_lock<std::mutex> lock(_mutex);
            cv_to_producers.notify_all();
            lock.unlock();
	    }
	    catch(const std::exception& e)
	    {
		    std::cerr << e.what() << '\n';
	    }
    }
}

void App::run()
{

    std::cout << "run\n";
    std::thread consumer(&App::consumer, this);
    std::thread _stdin(&App::stdin_handler, this);

    std::thread producer1(&App::two_words, this, 0, 2);
    std::thread producer2(&App::two_words, this, 1, 2);
    std::thread producer3(&App::producer1, this);
    std::thread producer4(&App::producer2, this);
    std::thread producer5(&App::numbers_both_sides, this);

    thread_pool.emplace_back(std::move(producer1));
    thread_pool.emplace_back(std::move(producer2));
    thread_pool.emplace_back(std::move(producer3));
    thread_pool.emplace_back(std::move(producer4));
    thread_pool.emplace_back(std::move(producer5));

    for (auto &thread : thread_pool)
    {
        thread.join();
        std::cout << "joined in run" << "\n";
    }


    std::cout << "producers joined \n";
    std::unique_lock<std::mutex> lock(_mutex);
    stop_consumer = true;
    cv_to_consumer.notify_one();
    lock.unlock();

    std::cout << "number of guessed passwords: " << cracked.size() << '\n';
    lock.lock();
    stop_consumer = true;
    lock.unlock();

    consumer.join();
    std::exit(0);   //TODO - make this cleaner, using std::jthread
    //_stdin.join();
    //print_cracked();
}

void App::consumer()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        cv_to_consumer.wait(lock);
        if(stop_consumer == true)
        {
            lock.unlock();
            return;
        }
        cracked_count++;
        lock.unlock();
        std::cout << cracked[cracked_count - 1] << '\n';
    }
}

void App::hash_and_find(const std::string &word)
{
    char md5[33];
    bytes2md5(word.c_str(), word.size(), md5);

    for(size_t j = 0; j < data.size(); ++j)
		{
            if(reset_producers == true)
            {
                return;
            }
			if(data[j].isCracked == false)
			{

				if(strcmp(data[j].hash.c_str(), md5) == 0){
                    std::unique_lock<std::mutex> lock(_mutex);
					data[j].isCracked = true;
					data[j].password = word;
                    cracked.push_back(data[j]);
                    cv_to_consumer.notify_one();
                    lock.unlock();
					//std::cout << "account: " << data[j].email << " guessed pswd: " << data[j].password << '\n';
				}
			}
		}
}

void App::numbers_both_sides() // 100*N executions for N elements in dictionary
{
    std::string number_a;
    std::string number_b;
    std::string temp;

    for(int a = 0; a < 10; ++a)
    {
	    number_a = std::to_string(a);
        for (int b = 0; b < 10; ++b)
        {
            number_b = std::to_string(b);
            for(size_t i = 0; i < dictionary.size(); ++i)
	        {
                while(reset_producers == true)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    cv_to_producers.wait(lock);
                    number_a = "0";
                    number_b = "0";
                    i = 0;
                    lock.unlock();
                    break;
                }
		        temp = number_a+dictionary[i]+number_b;
		        hash_and_find(temp);
	        }
        }
    }
}

void App::two_words(const int start, const int jump) // N*N executions for N elements in dictionary
{
    std::string temp;

    for(size_t i = start; i < dictionary.size(); i += jump)
	{
        for (size_t j = 0; j < dictionary.size(); ++j)
        {
            while(reset_producers == true)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                cv_to_producers.wait(lock);
                i = 0;
                j = 0;
                lock.unlock();
                break;
            }
            temp = dictionary[i] + " " + dictionary[j];
		    hash_and_find(temp);
        }
	}
}

void App::producer1() //nc,fu,au
{
    std::string temp;

    for(size_t i = 0; i < dictionary.size(); ++i)
	{
        while(reset_producers == true)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            cv_to_producers.wait(lock);
            i = 0;
            lock.unlock();
            break;
        }
        //no change
        hash_and_find(dictionary[i]);

        //first letter uppercase
        temp = dictionary[i];
        if(std::isalpha(static_cast<unsigned char>(temp[0])) != 0)
        {
            temp[0] = toupper(temp[0]);
            hash_and_find(temp);
        }

        //all upper
        std::transform(temp.cbegin(), temp.cend(), temp.begin(), ::toupper);
        hash_and_find(temp);
    }
}

void App::producer2() //naf, nab
{
    std::string number;
    std::string temp;

    for(int a = 0; a < 10; ++a)
    {
        number = std::to_string(a);

	    for(size_t i = 0; i < dictionary.size(); ++i)
	    {
            while(reset_producers == true)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                cv_to_producers.wait(lock);
                i = 0;
                number = "0";
                lock.unlock();
                break;
            }

		    temp = dictionary[i]+number;
		    hash_and_find(temp);

            temp = dictionary[i]+number;
		    hash_and_find(temp);
	    }
    }
}


void App::no_change()   // N executions for N elements in dictionary
{
	for(size_t i = 0; i < dictionary.size(); ++i)
	{
        hash_and_find(dictionary[i]);
    }
}

void App::number_at_front() // 10*N executions for N elements in dictionary
{
    std::string number;
    std::string temp;
    for(int a = 0; a < 10; ++a)
    {
	    number = std::to_string(a);

	    for(size_t i = 0; i < dictionary.size(); ++i)
	    {
		    temp = number+dictionary[i];
		    hash_and_find(temp);
	    }
    }
}

void App::number_at_back()  // 10*N executions for N elements in dictionary
{
    std::string number;
    std::string temp;
    for(int a = 0; a < 10; ++a)
    {
	    number = std::to_string(a);

	    for(size_t i = 0; i < dictionary.size(); ++i)
	    {
		    temp = dictionary[i]+number;
		    hash_and_find(temp);
	    }
    }
}

void App::first_upper() // N executions for N elements in dictionary
{
    std::string temp;

	for(size_t i = 0; i < dictionary.size(); ++i)
	{
        temp = dictionary[i];
        temp[0] = toupper(temp[0]);
        hash_and_find(temp);
    }
}

void App::all_upper()   // N executions for N elements in dictionary
{
    std::string temp;

	for(size_t i = 0; i < dictionary.size(); ++i)
	{
        temp = dictionary[i];
        std::transform(temp.cbegin(), temp.cend(), temp.begin(), ::toupper);
        hash_and_find(temp);
    }
}


#endif // !MD5_CLASSES
