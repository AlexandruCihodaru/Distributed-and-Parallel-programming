#include "pch.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include "ThreadPool.h"

using namespace std;

int a[1002][1002], b[1002][1002], res[1002][1002], n, m, t;
int threads[] = { 1, 5, 50, 1000 };

void load_data(std::string filename)
{
	int i, j;
	ifstream fin(filename);

	fin >> n >> m;

	for (i = 0; i < n; i++)
	{
		for (j = 0; j < m; j++)
		{
			fin >> a[i][j];
			res[i][j] = 0;
		}
	}
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < m; j++)
		{
			fin >> b[i][j];
		}
	}
}

void mult_async()
{
	vector <future<int>> f;
	int i, j, k, l;

	for (int i = 0; i < min(n, t); ++i)
	{
		f.push_back(std::async([](int line)
		{
			for (int i = line; i < n; i += t)
			{
				for (int j = 0; j < n; j++)
				{
					for (int l = 0; l < n; l++)
					{
						res[i][j] += a[i][j] * b[j][l];
					}
				}
			}
			return line;
		}, i));
	}

	for (i = 0; i < min(n, t); i++)
	{
		f[i].get();
	}
}

void add_async()
{
	vector <future<int>> f;
	int i, j;

	for (int i = 0; i < min(n, t); i++)
	{
		f.push_back(std::async([](int line)
		{
			for (int i = line; i < n; i += t)
			{
				for (int j = 0; j < m; j++)
				{
					res[i][j] = a[i][j] + b[i][j];
				}
			}
			return line;
		}, i));
	}

	for (int i = 0; i < min(n, t); i++)
	{
		f[i].get();
	}
}

void get_stats_async()
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			t = threads[j];
			string filename_add = "add" + std::to_string(i) + ".in";
			string filename_mult = "mult" + std::to_string(i) + ".in";
			load_data(filename_add);
			auto start = std::chrono::high_resolution_clock::now();
			add_async();
			auto finish = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double> time_el = finish - start;

			cout << "for " << threads[i] << "threads addition took " << time_el.count() << " using futures" << endl;

			load_data(filename_mult);
			auto start1 = std::chrono::high_resolution_clock::now();
			mult_async();
			auto finish1 = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double> time_el1 = finish1 - start1;

			cout << "for " << threads[i] << "threads multiplication took " << time_el1.count() << " using futures" << endl << endl;
		}
	}
}