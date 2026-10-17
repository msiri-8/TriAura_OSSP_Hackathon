#include <stdio.h>
#include <stdlib.h>

#define MAX_FRAMES 100
#define MAX_PAGES 1000
#define MAX_TESTS 5

typedef struct {
    int faults;
    int hits;
} Result;

typedef enum {
    FIFO,
    LRU,
    SMART
} Algorithm;

const char *algorithm_name(Algorithm algorithm) {
    switch (algorithm) {
        case FIFO:  return "FIFO";
        case LRU:   return "LRU";
        case SMART: return "Smart";
        default:    return "Unknown";
    }
}

int find_page(const int frames[], int count, int page) {
    for (int i = 0; i < count; i++) {
        if (frames[i] == page)
            return i;
    }
    return -1;
}

Result simulate(const int pages[], int page_count,
                int frame_count, Algorithm algorithm,
                int show_trace) {
    int frames[MAX_FRAMES];
    int last_used[MAX_FRAMES] = {0};
    int frequency[MAX_FRAMES] = {0};
    int next_fifo = 0;

    Result result = {0, 0};

    for (int i = 0; i < frame_count; i++)
        frames[i] = -1;

    if (show_trace) {
        printf("\n--- %s ---\n", algorithm_name(algorithm));
        printf("Page\tFrames\t\t\tResult\n");
    }

    for (int t = 0; t < page_count; t++) {
        int page = pages[t];
        int slot = find_page(frames, frame_count, page);
        const char *status;

        if (slot != -1) {
            result.hits++;
            status = "HIT";
            last_used[slot] = t + 1;

            if (algorithm == SMART)
                frequency[slot]++;
        } else {
            result.faults++;
            status = "FAULT";

            /* Use an empty frame if available. */
            slot = find_page(frames, frame_count, -1);

            if (slot == -1) {
                if (algorithm == FIFO) {
                    slot = next_fifo;
                    next_fifo = (next_fifo + 1) % frame_count;
                } else if (algorithm == LRU) {
                    slot = 0;

                    for (int j = 1; j < frame_count; j++) {
                        if (last_used[j] < last_used[slot])
                            slot = j;
                    }
                } else {
                    /*
                     * Smart policy:
                     * 1. Evict the lowest-frequency page.
                     * 2. On a tie, evict the least recently used.
                     */
                    slot = 0;

                    for (int j = 1; j < frame_count; j++) {
                        if (frequency[j] < frequency[slot] ||
                            (frequency[j] == frequency[slot] &&
                             last_used[j] < last_used[slot])) {
                            slot = j;
                        }
                    }
                }
            }

            frames[slot] = page;
            last_used[slot] = t + 1;
            frequency[slot] = 1;
        }

        if (show_trace) {
            printf("%d\t", page);

            for (int j = 0; j < frame_count; j++) {
                if (frames[j] == -1)
                    printf("- ");
                else
                    printf("%d ", frames[j]);
            }

            printf("\t%s\n", status);
        }
    }

    return result;
}

void print_comparison(const int pages[], int page_count,
                      int frame_count, int show_trace) {
    Result results[3];

    results[0] = simulate(pages, page_count, frame_count, FIFO,
                          show_trace);
    results[1] = simulate(pages, page_count, frame_count, LRU,
                          show_trace);
    results[2] = simulate(pages, page_count, frame_count, SMART,
                          show_trace);

    printf("\n========== PERFORMANCE COMPARISON ==========\n");
    printf("%-12s %10s %10s %12s\n",
           "Algorithm", "Faults", "Hits", "Hit Ratio");
    printf("----------------------------------------------------\n");

    for (int i = 0; i < 3; i++) {
        double ratio = (double)results[i].hits / page_count * 100.0;

        printf("%-12s %10d %10d %11.2f%%\n",
               algorithm_name((Algorithm)i),
               results[i].faults,
               results[i].hits,
               ratio);
    }

    int min_faults = results[0].faults;

    for (int i = 1; i < 3; i++) {
        if (results[i].faults < min_faults)
            min_faults = results[i].faults;
    }

    printf("\nLowest page fault count: %d\n", min_faults);
    printf("Best result(s): ");

    int first = 1;
    for (int i = 0; i < 3; i++) {
        if (results[i].faults == min_faults) {
            if (!first)
                printf(", ");
            printf("%s", algorithm_name((Algorithm)i));
            first = 0;
        }
    }

    printf("\n");
}

void manual_mode(void) {
    int pages[MAX_PAGES];
    int page_count, frame_count;

    printf("\nNumber of RAM frames (1-%d): ", MAX_FRAMES);
    if (scanf("%d", &frame_count) != 1 ||
        frame_count < 1 || frame_count > MAX_FRAMES) {
        printf("Invalid frame count.\n");
        return;
    }

    printf("Number of page requests (1-%d): ", MAX_PAGES);
    if (scanf("%d", &page_count) != 1 ||
        page_count < 1 || page_count > MAX_PAGES) {
        printf("Invalid page request count.\n");
        return;
    }

    printf("Enter %d non-negative page numbers:\n", page_count);

    for (int i = 0; i < page_count; i++) {
        if (scanf("%d", &pages[i]) != 1 || pages[i] < 0) {
            printf("Invalid page number.\n");
            return;
        }
    }

    print_comparison(pages, page_count, frame_count, 1);
}

void benchmark_mode(void) {
    const int test1[] = {1, 2, 3, 1, 4, 2, 5, 1};
    const int test2[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2};
    const int test3[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    const int test4[] = {1, 1, 1, 2, 2, 3, 3, 1, 1, 2, 3};
    const int test5[] = {4, 3, 2, 1, 4, 3, 5, 4, 3, 2, 1, 5};

    const int *tests[MAX_TESTS] = {
        test1, test2, test3, test4, test5
    };

    const int lengths[MAX_TESTS] = {
        sizeof(test1) / sizeof(test1[0]),
        sizeof(test2) / sizeof(test2[0]),
        sizeof(test3) / sizeof(test3[0]),
        sizeof(test4) / sizeof(test4[0]),
        sizeof(test5) / sizeof(test5[0])
    };

    const int frame_counts[MAX_TESTS] = {3, 3, 3, 3, 4};

    int total_faults[3] = {0, 0, 0};
    int smart_wins = 0;
    int smart_ties = 0;

    printf("\n============================================\n");
    printf("       AUTOMATIC BENCHMARK: 5 TESTS\n");
    printf("============================================\n");

    for (int t = 0; t < MAX_TESTS; t++) {
        Result results[3];

        printf("\nTest %d | Frames: %d | Requests: %d\n",
               t + 1, frame_counts[t], lengths[t]);

        printf("Sequence: ");
        for (int i = 0; i < lengths[t]; i++)
            printf("%d ", tests[t][i]);
        printf("\n");

        for (int a = 0; a < 3; a++) {
            results[a] = simulate(tests[t], lengths[t],
                                  frame_counts[t], (Algorithm)a, 0);
            total_faults[a] += results[a].faults;
        }

        printf("%-12s %10s %10s %12s\n",
               "Algorithm", "Faults", "Hits", "Hit Ratio");

        for (int a = 0; a < 3; a++) {
            double ratio =
                (double)results[a].hits / lengths[t] * 100.0;

            printf("%-12s %10d %10d %11.2f%%\n",
                   algorithm_name((Algorithm)a),
                   results[a].faults,
                   results[a].hits,
                   ratio);
        }

        int best = results[0].faults;
        for (int a = 1; a < 3; a++) {
            if (results[a].faults < best)
                best = results[a].faults;
        }

        if (results[SMART].faults == best) {
            int tied = 0;
            for (int a = 0; a < 3; a++) {
                if (a != SMART && results[a].faults == best)
                    tied = 1;
            }

            if (tied)
                smart_ties++;
            else
                smart_wins++;
        }
    }

    printf("\n============================================\n");
    printf("           OVERALL BENCHMARK\n");
    printf("============================================\n");
    printf("%-12s %15s\n", "Algorithm", "Total Faults");

    for (int a = 0; a < 3; a++)
        printf("%-12s %15d\n",
               algorithm_name((Algorithm)a), total_faults[a]);

    printf("\nSmart was the sole winner in %d test(s).\n", smart_wins);
    printf("Smart tied for the best in %d test(s).\n", smart_ties);
    printf("These are fixed test cases, not proof of universal superiority.\n");
}

int main(void) {
    int choice;

    printf("========================================\n");
    printf("  SMART PAGE REPLACEMENT SYSTEM\n");
    printf("========================================\n");
    printf("1. Manual simulation\n");
    printf("2. Automatic benchmark (5 tests)\n");
    printf("Choose an option: ");

    if (scanf("%d", &choice) != 1) {
        printf("Invalid input.\n");
        return 1;
    }

    switch (choice) {
        case 1:
            manual_mode();
            break;
        case 2:
            benchmark_mode();
            break;
        default:
            printf("Choose option 1 or 2.\n");
            return 1;
    }

    return 0;
}
