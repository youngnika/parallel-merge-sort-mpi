#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void merge(int *a, int left, int mid, int right, int *temp)
{
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right) {
        if (a[i] <= a[j])
            temp[k++] = a[i++];
        else
            temp[k++] = a[j++];
    }

    while (i <= mid)
        temp[k++] = a[i++];

    while (j <= right)
        temp[k++] = a[j++];

    for (i = left; i <= right; i++)
        a[i] = temp[i];
}

void merge_sort_rec(int *a, int left, int right, int *temp)
{
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;

    merge_sort_rec(a, left, mid, temp);
    merge_sort_rec(a, mid + 1, right, temp);
    merge(a, left, mid, right, temp);
}

void merge_sort(int *a, int n)
{
    if (n <= 1)
        return;

    int *temp = malloc((size_t)n * sizeof(int));

    if (!temp)
        MPI_Abort(MPI_COMM_WORLD, 1);

    merge_sort_rec(a, 0, n - 1, temp);
    free(temp);
}

int *allocate_ints(int n)
{
    int *p = malloc((size_t)(n > 0 ? n : 1) * sizeof(int));

    if (!p) {
        fprintf(stderr, "Memory allocation failed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    return p;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2) {
        if (rank == 0)
            fprintf(stderr, "Usage: %s N\n", argv[0]);

        MPI_Finalize();
        return 1;
    }

    char *end;
    long input_n = strtol(argv[1], &end, 10);

    if (*end != '\0' || input_n < 1 || input_n > 100000000) {
        if (rank == 0)
            fprintf(stderr, "N must be between 1 and 100000000\n");

        MPI_Finalize();
        return 1;
    }

    int n = (int)input_n;

    int *counts = allocate_ints(size);
    int *displs = allocate_ints(size);

    int base = n / size;
    int rem = n % size;
    int offset = 0;

    for (int i = 0; i < size; i++) {
        counts[i] = base + (i < rem ? 1 : 0);
        displs[i] = offset;
        offset += counts[i];
    }

    int local_n = counts[rank];
    int *local = allocate_ints(local_n);
    int *data = NULL;

    if (rank == 0) {
        data = allocate_ints(n);

        unsigned int seed = 123456789u;
        for (int i = 0; i < n; i++) {
            seed = seed * 1664525u + 1013904223u;
            data[i] = (int)(seed % 1000003u);
        }

        if (n <= 32) {
            printf("Original: ");
            for (int i = 0; i < n; i++)
                printf("%d ", data[i]);
            printf("\n");
        }
    }

    double scatter_time = 0.0;
    double sort_time = 0.0;
    double send_time = 0.0;
    double recv_time = 0.0;
    double merge_time = 0.0;
    long long merged_elements = 0;
    long long sent_elements = 0;

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();
    double phase_start = start;

    MPI_Scatterv(data, counts, displs, MPI_INT,
                 local, local_n, MPI_INT,
                 0, MPI_COMM_WORLD);

    scatter_time = MPI_Wtime() - phase_start;

    phase_start = MPI_Wtime();
    merge_sort(local, local_n);
    sort_time = MPI_Wtime() - phase_start;

    for (int step = 1; step < size; step *= 2) {
        if (rank % (2 * step) == step) {
            int dest = rank - step;

            phase_start = MPI_Wtime();
            MPI_Send(&local_n, 1, MPI_INT,
                     dest, 0, MPI_COMM_WORLD);

            MPI_Send(local, local_n, MPI_INT,
                     dest, 1, MPI_COMM_WORLD);
            send_time += MPI_Wtime() - phase_start;
            sent_elements += local_n;

            break;
        }

        if (rank % (2 * step) == 0 && rank + step < size) {
            int recv_n;
            int src = rank + step;

            phase_start = MPI_Wtime();
            MPI_Recv(&recv_n, 1, MPI_INT,
                     src, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);

            int *received = allocate_ints(recv_n);

            MPI_Recv(received, recv_n, MPI_INT,
                     src, 1, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            recv_time += MPI_Wtime() - phase_start;

            phase_start = MPI_Wtime();
            int *merged = allocate_ints(local_n + recv_n);

            int i = 0, j = 0, k = 0;

            while (i < local_n && j < recv_n) {
                if (local[i] <= received[j])
                    merged[k++] = local[i++];
                else
                    merged[k++] = received[j++];
            }

            while (i < local_n)
                merged[k++] = local[i++];

            while (j < recv_n)
                merged[k++] = received[j++];

            free(local);
            free(received);

            local = merged;
            local_n += recv_n;
            merged_elements += local_n;
            merge_time += MPI_Wtime() - phase_start;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double elapsed = MPI_Wtime() - start;
    double max_time = 0.0;

    MPI_Reduce(&elapsed, &max_time, 1,
               MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD);

    double phases[5] = {
        scatter_time, sort_time, send_time,
        recv_time, merge_time
    };
    double max_phases[5] = {0};

    MPI_Reduce(phases, max_phases, 5, MPI_DOUBLE,
               MPI_MAX, 0, MPI_COMM_WORLD);

    long long local_stats[2] = {
        merged_elements, sent_elements
    };
    long long total_stats[2] = {0};

    MPI_Reduce(local_stats, total_stats, 2,
               MPI_LONG_LONG_INT, MPI_SUM,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("PROFILE N=%d P=%d Scatter=%.6f Sort=%.6f "
               "Send=%.6f Recv=%.6f Merge=%.6f "
               "MergedElements=%lld SentElements=%lld\\n",
               n, size,
               max_phases[0], max_phases[1],
               max_phases[2], max_phases[3],
               max_phases[4],
               total_stats[0], total_stats[1]);

        int sorted = (local_n == n);

        for (int i = 1; i < local_n; i++) {
            if (local[i - 1] > local[i]) {
                sorted = 0;
                break;
            }
        }

        if (n <= 32) {
            printf("Sorted: ");
            for (int i = 0; i < local_n; i++)
                printf("%d ", local[i]);
            printf("\n");
        }

        printf("N=%d P=%d Time=%.6f s Correct=%s\n",
               n, size, max_time,
               sorted ? "YES" : "NO");
    }

    free(data);
    free(local);
    free(counts);
    free(displs);

    MPI_Finalize();
    return 0;
}
