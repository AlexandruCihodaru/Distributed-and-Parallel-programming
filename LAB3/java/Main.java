import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;
import java.util.concurrent.*;

public class Main {

    static int[] T = {1, 5, 50, 1000};
    static String[] adds = {"add1.in", "add2.in", "add3.in", "add4.in"};
    static String[] mults = {"mult1.in", "mult2.in", "mult3.in", "mult4.in"};

    public static void main(String[] args){
        int n, m;
        long startTime, finishTime;
        String filename;

        System.out.println("\t\tMultiplication");
        for(int i = 0; i < 4; i++){
            System.out.println("\tThreads :" + T[i]);
            filename = mults[i];
            try{
                Scanner in = new Scanner(new File(filename));
                n = in.nextInt();
                m = in.nextInt();
                int a[][] = new int[n][m];
                for (int j = 0; j < n; ++j) {
                    for (int k = 0; k < m; ++k) {
                        a[j][k] = in.nextInt();
                    }
                }
                int b[][] = new int[n][m];
                for (int j = 0; j < n; ++j) {
                    for (int k = 0; k < m; ++k) {
                        b[j][k] = in.nextInt();
                    }
                }
                int prod[][] = new int[n][m];

                /////////////ThreadPool
                for(int th = 0; th < 4; th++) {
                    ExecutorService executor = Executors.newFixedThreadPool(T[th]);
                    startTime = System.currentTimeMillis();
                    int thread;
                    int lastend = -1;
                    List<Future<?>> futures = new ArrayList<>();
                    for (thread = 0; thread < T[th]; thread++) {
                        int start = lastend + 1;
                        int finish = start + n / T[th] - 1;

                        if (n % T[th] > thread) {
                            finish++;
                        }
                        lastend = finish;
                        int finalFinish = finish;
                        Integer finalRowsSecond = n;
                        Integer finalColumnsSecond = n;
                        Runnable r = () -> {
                            if (start <= finalFinish) {
                                for (int q = start; q <= finalFinish; q++) {
                                    for (int w = 0; w < finalRowsSecond; w++) {
                                        for (int k = 0; k < finalColumnsSecond; k++) {
                                            prod[q][k] += a[q][w] * b[w][k];
                                        }
                                    }
                                }
                            }
                        };
                        futures.add(executor.submit(r));
                    }

                    // wait for all futures
                    futures.forEach(f -> {
                        try {
                            f.get();
                        } catch (InterruptedException | ExecutionException e) {
                            e.printStackTrace();
                        }
                    });

                    executor.shutdown();
                    finishTime = System.currentTimeMillis();

                    System.out.println("[POOL]  Time - " + (finishTime - startTime) / 1000.0);


                    ////////////////////////Futures
                    startTime = System.currentTimeMillis();
                    lastend = -1;
                    futures = new ArrayList<>();
                    for (thread = 0; thread < T[th]; thread++) {
                        int start = lastend + 1;
                        int finish = start + n / T[th] - 1;

                        if (n % T[th] > thread) {
                            finish++;
                        }
                        lastend = finish;
                        int finalFinish = finish;
                        Integer finalRowsSecond = n;
                        Integer finalColumnsSecond = n;
                        CompletableFuture<Object> future = CompletableFuture.supplyAsync(() -> {
                            if (start <= finalFinish) {
                                for (int q = start; q <= finalFinish; q++) {
                                    for (int j = 0; j < finalRowsSecond; j++) {
                                        for (int k = 0; k < finalColumnsSecond; k++) {
                                            prod[q][k] += a[q][j] * b[j][k];
                                        }
                                    }
                                }
                            }
                            return null;
                        });
                        futures.add(future);
                    }

                    // wait for all futures
                    futures.forEach(f -> {
                        try {
                            f.get();
                        } catch (InterruptedException | ExecutionException e) {
                            e.printStackTrace();
                        }
                    });

                    finishTime = System.currentTimeMillis();

                    System.out.println("[FUTURE]Time - " + (finishTime - startTime) / 1000.0);
                }
            }catch (Exception e) {
                System.out.println(e.toString());
                System.exit(1);
            }

        }


        System.out.println("\t\tAddition");
        for(int i = 0; i < 4; i++){
            System.out.println("\tThreads :" + T[i] );
            filename = adds[i];
            try{
                Scanner in = new Scanner(new File(filename));
                n = in.nextInt();
                m = in.nextInt();
                int a[][] = new int[n][m];
                for (int j = 0; j < n; ++j) {
                    for (int k = 0; k < m; ++k) {
                        a[j][k] = in.nextInt();
                    }
                }
                int b[][] = new int[n][m];
                for (int j = 0; j < n; ++j) {
                    for (int k = 0; k < m; ++k) {
                        b[j][k] = in.nextInt();
                    }
                }
                int sum[][] = new int[n][m];

                /////////////ThreadPool
                for(int th = 0; th < 4; th++) {
                    ExecutorService executor = Executors.newFixedThreadPool(T[th]);
                    startTime = System.currentTimeMillis();
                    int thread;
                    int lastend = -1;
                    List<Future<?>> futures = new ArrayList<>();
                    for (thread = 0; thread < T[th]; thread++) {
                        int start = lastend + 1;
                        int finish = start + n / T[th] - 1;

                        if (n % T[th] > thread) {
                            finish++;
                        }
                        lastend = finish;
                        int finalFinish = finish;
                        Integer finalRowsSecond = n;
                        Integer finalColumnsSecond = n;
                        Runnable r = () -> {
                            if (start <= finalFinish) {
                                for (int q = start; q <= finalFinish; q++) {
                                    for (int w = 0; w < finalRowsSecond; w++) {
                                        sum[q][w] = a[q][w] + b[q][w];
                                    }
                                }
                            }
                        };
                        futures.add(executor.submit(r));
                    }

                    // wait for all futures
                    futures.forEach(f -> {
                        try {
                            f.get();
                        } catch (InterruptedException | ExecutionException e) {
                            e.printStackTrace();
                        }
                    });

                    executor.shutdown();
                    finishTime = System.currentTimeMillis();

                    System.out.println("[POOL]  Time - " + (finishTime - startTime) / 1000.0);


                    ////////////////////////Futures
                    startTime = System.currentTimeMillis();
                    lastend = -1;
                    futures = new ArrayList<>();
                    for (thread = 0; thread < T[th]; thread++) {
                        int start = lastend + 1;
                        int finish = start + n / T[th] - 1;

                        if (n % T[th] > thread) {
                            finish++;
                        }
                        lastend = finish;
                        int finalFinish = finish;
                        Integer finalRowsSecond = n;
                        Integer finalColumnsSecond = n;
                        CompletableFuture<Object> future = CompletableFuture.supplyAsync(() -> {
                            if (start <= finalFinish) {
                                for (int q = start; q <= finalFinish; q++) {
                                    for (int j = 0; j < finalRowsSecond; j++) {
                                        sum[q][j] = a[q][j] + b[q][j];
                                    }
                                }
                            }
                            return null;
                        });
                        futures.add(future);
                    }

                    // wait for all futures
                    futures.forEach(f -> {
                        try {
                            f.get();
                        } catch (InterruptedException | ExecutionException e) {
                            e.printStackTrace();
                        }
                    });

                    finishTime = System.currentTimeMillis();

                    System.out.println("[FUTURE]Time - " + (finishTime - startTime) / 1000.0);
                }
            }catch (Exception e) {
                System.out.println(e.toString());
                System.exit(1);
            }

        }
    }
}
