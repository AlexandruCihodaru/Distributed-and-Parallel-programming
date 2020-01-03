#include "pch.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <assert.h>
#include <algorithm>
#include <string>

using namespace std;

const int maxlg = 22;
const int maxn = 500005;

int n;
long long dp[maxlg][maxn], sum[maxn], psum[maxn];

const int T = 5;

//#define debug

void reinitialise()
{
	for (int i = 0; i < maxlg; i++)
	{
		for (int j = 0; j < maxn; j++)
		{
			dp[i][j] = 0;
		}
	}

	for (int j = 0; j < maxn; j++)
	{
		sum[j] = psum[j] = 0;
	}
}

inline void doIt(int idx) {
	for (int i = idx; i < n; i += T) {
		int act = 0;
		int now = i + 1;
		for (int bit = 0; (1 << bit) <= now; ++bit) {
			if (now & (1 << bit)) {
				sum[i] += dp[bit][act];
				act += (1 << bit);
			}
		}
	}
}

int main() {
	clock_t t;

	for (int zz = 1; zz < 7; zz++)
	{
		reinitialise();
		ifstream fin(to_string(zz) + ".txt");

		fin >> n;

		fin >> dp[0][0];
		psum[0] = dp[0][0];
		for (int i = 1; i < n; ++i)
		{
			fin >> dp[0][i];
			psum[i] = psum[i - 1] + dp[0][i];
		}
		/// dp[k][i] = sum(a[j]) where i <= j <= i + 2^k - 1
		
		t = clock();
		// dp[k][i] =
		for (int k = 1; (1 << k) < maxn; ++k) {
			for (int i = 0; i < n; ++i) {
				dp[k][i] = dp[k - 1][i] + dp[k - 1][i + (1 << (k - 1))];
			}
		}

		vector <thread> th;
		for (int i = 0; i < min(T, n); ++i) {
			th.push_back(thread(doIt, i));
		}
		for (int i = 0; i < th.size(); ++i) {
			th[i].join();
		}

		
		for (int i = 0; i < n; ++i) {
			assert(sum[i] == psum[i]);
		}

		t = clock() - t;
		cout << "Computing partial sums of an array with " << n
			<< " elements took me " << t << " cycles ("
			<< static_cast<float> (t) / CLOCKS_PER_SEC << " seconds)\n";
	}
	return 0;
}