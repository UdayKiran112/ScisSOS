#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define ALGO_COUNT 4
#define NUM_RUNS 10

const char *algorithms[ALGO_COUNT] = {"fcfs", "sjf", "priority", "rr"};

double run_once(const char *algo)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child: suppress output
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull == -1)
        {
            perror("open /dev/null failed");
            exit(1);
        }
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);

        execl("./run_os", "run_os", algo, (char *)NULL);
        perror("execl failed");
        exit(1);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        clock_gettime(CLOCK_MONOTONIC, &end);

        return (end.tv_sec - start.tv_sec) +
               (end.tv_nsec - start.tv_nsec) / 1e9;
    }
    else
    {
        perror("fork failed");
        return -1.0;
    }
}

void benchmark_algorithm(const char *algo)
{
    double total = 0.0, min = 1e9, max = 0.0;
    double time;

    printf("Benchmarking: %s\n", algo);

    for (int i = 0; i < NUM_RUNS; i++)
    {
        time = run_once(algo);
        if (time < 0)
        {
            fprintf(stderr, "Error in run #%d for %s\n", i + 1, algo);
            continue;
        }
        total += time;
        if (time < min)
            min = time;
        if (time > max)
            max = time;
    }

    printf("→ Average time: %.6f seconds (over %d runs)\n", total / NUM_RUNS, NUM_RUNS);
    printf("→ Min time:     %.6f seconds\n", min);
    printf("→ Max time:     %.6f seconds\n\n", max);
}

int main()
{
    for (int i = 0; i < ALGO_COUNT; i++)
    {
        benchmark_algorithm(algorithms[i]);
    }
    return 0;
}
