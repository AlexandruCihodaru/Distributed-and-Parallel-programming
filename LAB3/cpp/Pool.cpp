#include "pch.h"
#include <string>
#include <fstream>
#include <iostream>
#include "ThreadPool.h"

using namespace std;

int a_pool[1002][1002], b_pool[1002][1002], res_pool[1002][1002], n_pool, m_pool, t_pool;
int threads_pool[] = { 1, 5, 50, 1000 };

void load_data_pool(std::string filename)
{
	int i, j;
	ifstream fin(filename);

	fin >> n_pool >> m_pool;

	for (i = 0; i < n_pool; i++)
	{
		for (j = 0; j < m_pool; j++)
		{
			fin >> a_pool[i][j];
			res_pool[i][j] = 0;
		}
	}
	for (i = 0; i < n_pool; i++)
	{
		for (j = 0; j < m_pool; j++)
		{
			fin >> b_pool[i][j];
		}
	}
}

void mult_pool()
{
	ThreadPool pool(5);
	vector <future<int>> f;

	for (int i = 0; i < min(n_pool, t_pool); i++)
	{
		f.push_back(pool.enqueue([](int line)
		{
			for (int i = line; i < n_pool; i += t_pool)
			{
				for (int j = 0; j < n_pool; j++)
				{
					for (int l = 0; l < n_pool; ++l)
					{
						res_pool[i][j] += a_pool[i][j] * b_pool[j][l];
					}
				}
			}
			return line;
		}, i));
	}

	for (int i = 0; i < min(n_pool, t_pool); i++)
	{
		f[i].get();
	}
}

void add_pool()
{
	ThreadPool pool(5);
	vector <future<int>> f;

	for (int i = 0; i < min(n_pool, t_pool); i++)
	{
		f.push_back(pool.enqueue([](int line)
		{
			for (int i = line; i < n_pool; i += t_pool)
			{
				for (int j = 0; j < m_pool; j++)
				{
					res_pool[i][j] = a_pool[i][j] + b_pool[i][j];
				}
			}
			return line;
		}, i));
	}
	for (int i = 0; i < min(n_pool, t_pool); i++)
	{
		f[i].get();
	}
}

void get_stats_pool()
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			t_pool = threads_pool[j];
			string filename_add = "add" + std::to_string(i) + ".in";
			string filename_mult = "mult" + std::to_string(i) + ".in";
			load_data_pool(filename_add);
			auto start = std::chrono::high_resolution_clock::now();
			add_pool();
			auto finish = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double> time_el = finish - start;

			cout << "for " << threads_pool[i] << "threads addition took " << time_el.count() << " using pools" << endl;

			load_data_pool(filename_mult);
			auto start1 = std::chrono::high_resolution_clock::now();
			mult_pool();
			auto finish1 = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double> time_el1 = finish1 - start1;

			cout << "for " << threads_pool[i] << "threads multiplication took " << time_el1.count() << " using pools" << endl << endl;
		}
	}
}