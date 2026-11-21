#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static FILE *trace;
static unsigned long long access_id = 0;
static int array_size;

static void log_access(char array, char operation, int index)
{
    unsigned long long address;

    if (array == 'A')
        address = (unsigned long long)index;
    else
        address = (unsigned long long)array_size + index;

    fprintf(trace, "%llu,%llu,%c,%c,%d\n",
            access_id++, address, array, operation, index);
}

static int read_element(int *a, char name, int index)
{
    log_access(name, 'R', index);
    return a[index];
}

static void write_element(int *a, char name, int index, int value)
{
    log_access(name, 'W', index);
    a[index] = value;
}

static void merge(int *a, int *temp, int left, int mid, int right)
{
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right) {
        int x = read_element(a, 'A', i);
        int y = read_element(a, 'A', j);

        if (x <= y) {
            write_element(temp, 'T', k++, x);
            i++;
        } else {
            write_element(temp, 'T', k++, y);
            j++;
        }
    }

    while (i <= mid) {
        int x = read_element(a, 'A', i++);
        write_element(temp, 'T', k++, x);
    }

    while (j <= right) {
        int y = read_element(a, 'A', j++);
        write_element(temp, 'T', k++, y);
    }

    for (int p = left; p <= right; p++) {
        int x = read_element(temp, 'T', p);
        write_element(a, 'A', p, x);
    }
}

static void merge_sort(int *a, int *temp, int left, int right)
{
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;

    merge_sort(a, temp, left, mid);
    merge_sort(a, temp, mid + 1, right);
    merge(a, temp, left, mid, right);
}

int main(int argc, char **argv)
{
    int n = 1024;

    if (argc > 1) {
        char *end;
        long value = strtol(argv[1], &end, 10);

        if (*end != '\0' || value < 1 || value > 100000) {
            fprintf(stderr, "N must be between 1 and 100000\n");
            return 1;
        }

        n = (int)value;
    }

    array_size = n;

    int *a = malloc((size_t)n * sizeof(int));
    int *temp = malloc((size_t)n * sizeof(int));

    if (!a || !temp) {
        fprintf(stderr, "Memory allocation failed\n");
        free(a);
        free(temp);
        return 1;
    }

    unsigned int seed = 123456789u;

    for (int i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        a[i] = (int)(seed % 1000003u);
    }

    trace = fopen("memory_trace.csv", "w");

    if (!trace) {
        perror("memory_trace.csv");
        free(a);
        free(temp);
        return 1;
    }

    fprintf(trace, "step,address,array,operation,index\n");

    merge_sort(a, temp, 0, n - 1);

    fclose(trace);

    int correct = 1;

    for (int i = 1; i < n; i++) {
        if (a[i - 1] > a[i])
            correct = 0;
    }

    printf("N=%d Accesses=%llu Correct=%s\n",
           n, access_id, correct ? "YES" : "NO");

    free(a);
    free(temp);

    return correct ? 0 : 1;
}
