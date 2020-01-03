// Lab6.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <vector>

using namespace std;

typedef vector <long long> POLYN;

typedef enum _TIME_ELAPSED_STATE
{
    START_MEASURE,
    END_MEASURE
}TIME_ELAPSED_STATE;

static unsigned int gMaxSupportedThreads = std::thread::hardware_concurrency();

std::chrono::time_point<std::chrono::steady_clock> gStart;
std::chrono::time_point<std::chrono::steady_clock> gEnd;
std::chrono::duration<double> gElapsedTime;

static
void
PrintPoly(
    _In_ const POLYN& Pol
    )
{
    size_t n = Pol.size();
    for (size_t i = 0; i < n; i++)
    {
        if (Pol[i] == 0) continue;

        cout << Pol[i];
        if (i != 0)
            cout << "x^" << i;

        if (i != n - 1 && Pol[i + 1] != 0)
            cout << " + ";
    }

    cout << endl;
}

static
void
PrintElapsedTime(
    void
    )
{
    std::cout << "----> Total elapsed time " << std::chrono::duration_cast<std::chrono::microseconds>(gEnd - gStart).count() << " microseconds\n";
}

static
void
MeasureElapsedTime(
    _In_ TIME_ELAPSED_STATE TimeState
    )
{
    switch (TimeState)
    {
    case START_MEASURE:
        gStart = std::chrono::steady_clock::now();
        break;
    case END_MEASURE:
        gEnd = std::chrono::steady_clock::now();
        PrintElapsedTime();
    }
}

static
void
PolynomMultiplicationSingleThreadedNaive(
    _In_  const POLYN &A,
    _In_  const POLYN &B,
    _Out_ POLYN &Prod
    )
{
    cout << "Polynomial multiplication single threaded NAIVE\n";
    std::fill(Prod.begin(), Prod.end(), 0);

    size_t n = A.size();
    size_t m = B.size();

    MeasureElapsedTime(START_MEASURE);
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = 0; j < m; j++)
        {
            // i + j is the power of x after A[i] * B[j] element
            Prod[i + j] = Prod[i + j] + A[i] * B[j];
        }
    }
    PrintPoly(Prod);
    MeasureElapsedTime(END_MEASURE);
}

static
void
_KaratsubaSingleThreaded(
    _In_ const POLYN &A,
    _In_ const POLYN &B,
    _Out_ POLYN &C
    )
{
    if (A.size() == 1 && B.size() == 1)
    {
        C[0] = A[0] * B[0];
        return;
    }

    size_t half = A.size() / 2;

    POLYN A0(A.begin(), A.begin() + half);    // A0
    POLYN A1(A.begin() + half, A.end());      // A1

    POLYN B0(B.begin(), B.begin() + half);    // B0
    POLYN B1(B.begin() + half, B.end());      // B1

    POLYN C1(A0.size() + B0.size() - 1);      // C1
    POLYN C2(A1.size() + B1.size() - 1);      // C2

    _KaratsubaSingleThreaded(A0, B0, C1);     // C1 = A0 * B0 - by recursive call
    _KaratsubaSingleThreaded(A1, B1, C2);     // C2 = A1 * B1 - by recursive call

    for (size_t i = 0; i < C1.size(); ++i)
    {
        C[i] += C1[i];                        // C = C1;
    }

    for (size_t i = 0; i < A1.size(); ++i)
    {
        A0[i] += A1[i];                       // C3 = A0 + A1
        B0[i] += B1[i];                       // C4 = B0 + B1
    }
    POLYN C5(A0.size() + B0.size() - 1);
    _KaratsubaSingleThreaded(A0, B0, C5);     // C5 = C3 * C4 - by recursive call

    for (size_t i = 0; i < C5.size(); ++i)          // At this point C = C1;
    {
        C[i + half] += C5[i] - C1[i] - C2[i];     // C += C6; => C = C1 + C6; where C6 = C5 - C1 - C2
    }

    for (size_t i = 0; i < C2.size(); ++i)          // At this point C = C1 + C6*X^(n/2)
    {
        C[i + 2 * half] += C2[i];                   // C = C1 + C6*X^(n/2) + C2*X^n
    }
}

static
void
PolynomMultiplicationSingleThreadedKaratsuba(
    _In_  POLYN &A,
    _In_  POLYN &B,
    _Out_ POLYN &Prod
)
{
    cout << "Polynomial multiplication single threaded KARATSUBA\n";

    std::fill(Prod.begin(), Prod.end(), 0);
    size_t sizeA = A.size();
    size_t sizeB = B.size();

    POLYN copyA = A;
    POLYN copyB = B;

    size_t maxSize = max(sizeA, sizeB);

    while (maxSize & (maxSize - 1))
    {
        // While not power of 2
        maxSize++;
    }

    A.resize(maxSize);
    B.resize(maxSize);

    Prod.resize(maxSize * 2 - 1);

    //cout << "Maxsize found: " << maxSize << endl;
    MeasureElapsedTime(START_MEASURE);
    _KaratsubaSingleThreaded(A, B, Prod);
    PrintPoly(Prod);
    MeasureElapsedTime(END_MEASURE);

    A = copyA;
    B = copyB;
}

int gN;
POLYN gA, gB, gProd2;
int gElementsForThreadToParse = 3;

void
_ThreadWorkerOnCoefficient(
    _In_ int CoefficientToStart
    )
{
    for (int i = CoefficientToStart; i < gN; i += gElementsForThreadToParse)
    {
        // eg for coefficient 3, we would have (a[0] * b[3]), (a[1] * b[2]), (a[2] * B[1]), (a[3] * b[0]))

        for (int coeffInA = 0; coeffInA <= i; ++coeffInA)
        {
            int coeffInB = i - coeffInA;
            gProd2[i] += (gA[coeffInA] * gB[coeffInB]);
        }
    }
}

static
void
PolynomMultiplicationMultiThreadedNaive(
    _In_  const POLYN &A,
    _In_  const POLYN &B
    )
{
    cout << "Polynomial multiplication multi threaded NAIVE\n";

    gA = A;
    gB = B;

    size_t sizeA = gA.size();
    size_t sizeB = gB.size();

    gN = sizeA * 2 - 1 ;

    gProd2.resize(gN, 0);
    gA.resize(gN, 0);
    gB.resize(gN, 0);

    fill(gProd2.begin(), gProd2.end(), 0);

    vector<thread> threads;
    MeasureElapsedTime(START_MEASURE);

    for (int i = 0; i < min(gN, gElementsForThreadToParse); i++)
    {
        threads.emplace_back(_ThreadWorkerOnCoefficient, i);
    }

    for (int i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }

    PrintPoly(gProd2);
    MeasureElapsedTime(END_MEASURE);

    gA = A;
    gB = B;
}

#define MAX_THREADS std::thread::hardware_concurrency()
int gCurrentThreads = 0;

static
void
_KaratsubaMultiThreaded(
    const POLYN &a,
    const POLYN &b,
    POLYN &C
    )
{
    if (a.size() == 1 && b.size() == 1)
    {
        C[0] = a[0] * b[0];
        return;
    }

    int half = a.size() / 2 + a.size() % 2;

    POLYN A0(a.begin(), a.begin() + half);  // A0
    POLYN A1(a.begin() + half, a.end());    // A1

    POLYN B0(b.begin(), b.begin() + half);  // B0
    POLYN B1(b.begin() + half, b.end());    // B1

    POLYN C1(A0.size() + B0.size() - 1);    // C1
    POLYN C2(A1.size() + B1.size() - 1);    // C2
    vector <thread> th;

    gCurrentThreads += 2;
    if (gCurrentThreads < MAX_THREADS)
    {
        th.emplace_back(_KaratsubaMultiThreaded, std::ref(A0), std::ref(B0), std::ref(C1));     // C1 = A0 * B0  (by thread)
        th.push_back(thread([&A1, &B1, &C2]() {_KaratsubaMultiThreaded(A1, B1, C2); }));        // C2 = A1 * B1  (by thread)
    }
    else {
        _KaratsubaMultiThreaded(A0, B0, C1);    // C1 = A0 * B0  (by recursive call)
        _KaratsubaMultiThreaded(A1, B1, C2);    // C2 = A1 * B1  (by recursive call)
    }

    // middle
    POLYN C3(A0);   // C3
    POLYN C4(B0);   // C4

    for (int i = 0; i < A1.size(); i++)
    {
        C3[i] += A1[i];                 // C3 = A0 + A1
        C4[i] += B1[i];                 // C4 = B0 + B1
    }

    POLYN C5(C3.size() + C4.size() - 1);

    gCurrentThreads++;

    if (gCurrentThreads < MAX_THREADS)
    {
        th.emplace_back(_KaratsubaMultiThreaded, std::ref(C3), std::ref(C4), std::ref(C5));
    }
    else
    {
        _KaratsubaMultiThreaded(C3, C4, C5);
    }

    // wait for threads to finish
    gCurrentThreads -= th.size();
    for (int i = 0; i < th.size(); i++)
    {
        th[i].join();
    }

    // copy first part
    for (int i = 0; i < C1.size(); i++)
    {
        C[i] += C1[i];
    }

    for (int i = 0; i < C5.size(); i++)
    {
        C[i + half] += C5[i] - C1[i] - C2[i];
    }

    for (int i = 0; i < C2.size(); i++)
    {
        C[i + 2 * half] += C2[i];
    }
}

static
void
PolynomMultiplicationMultiThreadedKaratsuba(
    _In_  POLYN &A,
    _In_  POLYN &B,
    _In_  POLYN &Prod
)
{
    cout << "Polynomial multiplication multi threaded KARATSUBA\n";

    std::fill(Prod.begin(), Prod.end(), 0);
    size_t sizeA = A.size();
    size_t sizeB = B.size();

    POLYN copyA = A;
    POLYN copyB = B;

    size_t maxSize = max(sizeA, sizeB);

    while (maxSize & (maxSize - 1))
    {
        // While not power of 2
        maxSize++;
    }

    A.resize(maxSize);
    B.resize(maxSize);

    Prod.resize(maxSize * 2 - 1, 0);

    MeasureElapsedTime(START_MEASURE);
    _KaratsubaMultiThreaded(A, B, Prod);
    PrintPoly(Prod);
    MeasureElapsedTime(END_MEASURE);

}

int main()
{
    // first polynom
    POLYN a = { 5, 0, 10, 6};

    // second polynom
    POLYN b = { 1, 2, 4, 3 };

    size_t sizeA = a.size();
    size_t sizeB = b.size();

    // Result polynom
    POLYN prod(sizeA + sizeB - 1);

    PolynomMultiplicationSingleThreadedNaive(a, b, prod);

    PolynomMultiplicationSingleThreadedKaratsuba(a, b, prod);

    PolynomMultiplicationMultiThreadedNaive(a, b);

    PolynomMultiplicationMultiThreadedKaratsuba(a, b, prod);
}
