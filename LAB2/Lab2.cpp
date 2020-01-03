// Lab2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <cassert>
#include <chrono>
#include <algorithm>
#include <string>

using namespace std;

#define do_add
//#define mult

const int MAXN = 2001;
const int MAXT[] = { 1, 3, 10, 30, 100, 300, 1000 };

int n, m, k;
int a[MAXN][MAXN], b[MAXN][MAXN], c[MAXN][MAXN];

void loadDataMultiplication(int a[MAXN][MAXN], int b[MAXN][MAXN], string fileName)
{
	ifstream fin(fileName);
	fin >> n >> m >> k;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < m; j++)
		{
			fin >> a[i][j];
		}
	}
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < m; j++)
		{
			fin >> b[i][j];
		}
	}
}

void loadData(int a[MAXN][MAXN], int b[MAXN][MAXN], string fileName) 
{
	ifstream fin(fileName);
	fin >> n >> m;
	for (int i = 0; i < n; i++) 
	{
		for (int j = 0; j < m; j++) 
		{
			fin >> a[i][j];
		}
	}
	for (int i = 0; i < n; i++) 
	{
		for (int j = 0; j < m; j++)
		{
			fin >> b[i][j];
		}
	}
}

void addLines(int line, int T)
{
	for (int i = line; i < n; i += T)
	{
		for (int j = 0; j < m; j++)
		{
			c[i][j] = a[i][j] + b[i][j];
		}
	}
}

void multLines(int line, int T)
{
	for (int i = line; i < n; i += T)
	{
		for (int j = 0; j < m; ++j)
		{
			for (int l = 0; l < k; ++l)
			{
				c[i][l] += a[i][j] * b[j][l];
			}
		}
	}
}

void doThing(int T)
{
	vector <thread> threads;
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < std::min(n, T); i++)
	{
#if defined do_add
		threads.push_back(thread(addLines, i, T));
#elif defined mult
		threads.push_back(thread(multLines, i, T));
#endif
	}

	for (int i = 0; i < std::min(n, T); i++)
	{
		threads[i].join();
	}

	auto finish = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> time_el = finish - start;

#if defined do_add
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < m; j++)
		{
			if (!(c[i][j] = a[i][j] + b[i][j]))
			{
				cout << "Error\n";
			}
		}
	}
#elif defined mult
	/*for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < m; j++)
		{
			cout << c[i][j] << " ";
		}
		cout << endl;
	}*/
#endif

	cout <<"\t"<< time_el.count()<<"\n";
}

void reinitialiseC()
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < m; j++)
		{
			c[i][j] = 0;
		}
	}
}

int main()
{
	for (int j = 1; j <= 5; j++)
	{
#if defined do_add
		loadData(a, b, "add" + std::to_string(j) + ".in");
#elif defined mult
		loadDataMultiplication(a, b, "add" + std::to_string(j) + ".in");
#endif

		cout << "set " << j << endl;
		for (int i = 0; i < 7; ++i)
		{
			int t = MAXT[i];
			cout << t << " threads\n";
			reinitialiseC();
			doThing(t);
		}
	}
	return 0;
}