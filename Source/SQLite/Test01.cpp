#include <iostream>
#include <functional>

#include <Utilities/External/sqlite3/sqlite3.h>

int callback(void* a_param, int argc, char** argv, char** column)
{
	for (int i = 0; i < argc; ++i)
		std::cout << argv[i] << ' ';

	std::cout << std::endl;

	return 0;
}

int main()
{
	try
	{
		sqlite3* db = nullptr;

		if (SQLITE_OK != sqlite3_open("E:/SQLite3/Directory.db", &db))
			throw std::exception(sqlite3_errmsg(db));

		const char* query =
			"select p.Name, p.Value from Parameters p "
			"where p.Name = \"goforward\";";

		char* errormsg = nullptr;

		sqlite3_exec(db, query, callback, nullptr, &errormsg);

		if (errormsg)
		{
			std::cout << errormsg << std::endl;
			sqlite3_free(errormsg);
		}

		sqlite3_close(db);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception : " << e.what() << std::endl;
	}	

	std::cin.get();
}