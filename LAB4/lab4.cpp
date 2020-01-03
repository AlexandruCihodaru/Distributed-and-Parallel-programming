#include "pch.h"
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <tuple>
#include <fstream>
#include <chrono>
#include <string>


#define debug
int a[1005][1005], b[1005][1005], c[1005][1005], res[1005][1005], n;
const int T[] = { 1, 5, 10, 30, 100, 1000 };
const std::string mult[] = { "mult1.in", "mult2.in", "mult3.in", "mult4.in" };

std::mutex mx;
std::condition_variable cv;
std::queue<std::tuple<int, int, int>> q;
bool finished;

void loadData(std::string name)
{
	std::ifstream fin(name);
	fin >> n;
	
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			fin >> a[i][j];
		}
	}

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			fin >> b[i][j];
		}
	}

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			fin >> c[i][j];
		}
	}
}

inline void producer(int line, int T)
{
	int i, j, k, aux_i_j;

	for (i = 0; i < n; i += T)
	{
		for (j = 0; j < n; j++)
		{
			aux_i_j = 0;
			for (k = 0; k < n; k++)
			{
				aux_i_j += a[i][k] * b[k][j];
			}

			{
				std::lock_guard<std::mutex>lk(mx);
				q.push(std::make_tuple(i, j, aux_i_j));
			}
			cv.notify_all();
		}
	}
}

inline void consumer(int tid, int T)
{
	while (true)
	{
		std::unique_lock<std::mutex> lk(mx);
		
		cv.wait(lk, [] {return finished || !q.empty(); });
		if (finished)
		{
			break;
		}
		
		std::tuple<int, int, int> el = q.front();
		q.pop();

		int i = std::get<0>(el);
		int j = std::get<1>(el);
		int x = std::get<2>(el);

		for (int k = 0; k < n; ++k)
		{
			res[i][k] += x * c[j][k];
		}
	}
}
int main()
{
	int j;

	for (j = 0; j < 4; j++)
	{
		loadData("mult" + std::to_string(j + 1) + ".in");
		for (int p = 0; p < 6; p++)
		{
			std::cout <<"test "<< j + 1 << " on size "<< n << " with " << T[p] << "threads" << std::endl;
			int t = T[p];
			auto start = std::chrono::high_resolution_clock::now();
			std::vector <std::thread> producers, consumers;

			for (int i = 0; i < std::min(n, t); ++i)
			{
				producers.push_back(std::thread(producer, i, t));
			}

			for (int i = 0; i < std::min(n, t); ++i)
			{
				consumers.push_back(std::thread(consumer, i, t));
			}

			for (int i = 0; i < producers.size(); ++i)
			{
				producers[i].join();
			}

			{
				std::lock_guard<std::mutex> lk(mx);
				finished = true;
			}

			cv.notify_all();
			for (int i = 0; i < consumers.size(); ++i)
			{
				consumers[i].join();
			}

			auto finish = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> total = finish - start;
			std::cout << (total).count() << "\n";
		}
	}
}