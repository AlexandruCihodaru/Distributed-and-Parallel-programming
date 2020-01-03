// Lab3Cpp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include "ThreadPool.h"

using namespace std;

int main()
{
	cout << "\t\tAsync\n\n";
	get_stats_async();
	cout << "\n\n\n\t\tPools\n\n";
	get_stats_pool();
	cout << "\n\n";
	return 0;
}

