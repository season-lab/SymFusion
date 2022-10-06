#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITMAP_SIZE (30 * 65536)

char bitmap_src[BITMAP_SIZE] = { 0 };
char bitmap_dst[BITMAP_SIZE] = { 0 };

int unique_counter = 0;
short uniques[BITMAP_SIZE] = { 0 };

static void load_bitmap(char* path, char* bitmap, int bitmap_size)
{
    // printf("Loading bitmap: %s\n", path);
    FILE* fp = fopen(path, "r");
    if (!fp) {
        printf("Cannot open: %s\n", path);
        exit(1);
    }
    int count = 0;
    while (count < bitmap_size) {
        int res = fread(bitmap + count, 1, bitmap_size - count, fp);
        if (res <= 0) break;
        count += res;
    }
    if (count < bitmap_size) {
        printf("Cannot read full bitmap: %s [%d]\n", path, count);
        exit(1);
    }
    fclose(fp);
}

static void save_bitmap(char* path, char* bitmap, int bitmap_size)
{
    // printf("Saving bitmap: %s\n", path);
    FILE* fp = fopen(path, "w");
    if (!fp) {
        printf("Cannot open: %s\n", path);
        exit(1);
    }
    int count = 0;
    while (count < bitmap_size) {
        int res = fwrite(bitmap + count, 1, bitmap_size - count, fp);
        if (res <= 0) break;
        count += res;
    }
    if (count < bitmap_size) {
        printf("Cannot write full bitmap: %s [%d]\n", path, count);
        exit(1);
    }
    fclose(fp);
}

static int compare_bitmaps(char* bitmap_src, char* bitmap_dst, char* report_file, int bitmap_size)
{
    int new_coverage = 0;
    int is_interesting = 0;
    for (int i = 0; i < bitmap_size; i++) {
        if ((bitmap_src[i] | bitmap_dst[i]) != bitmap_dst[i]) {
            if (report_file) uniques[unique_counter++] = (short) i;
            is_interesting = 1;
            if (bitmap_dst[i] == 0)
                new_coverage = 1; 
            bitmap_dst[i] |= bitmap_src[i];
            // printf("Unique ID: %d\n", i);
        }
    }
    if (is_interesting && report_file != NULL) {
        FILE* fp = fopen(report_file, "w");
        if (!fp) {
            printf("Cannot open: %s\n", report_file);
            exit(1);
        }
        int count = 0;
        while (count < (unique_counter * sizeof(short))) {
            int res = fwrite(uniques + count, 1, (unique_counter * sizeof(short)) - count, fp);
            if (res <= 0) break;
            count += res;
        }
        if (count < (unique_counter * sizeof(short))) {
            printf("Cannot write full bitmap: %s [%d]\n", report_file, count);
            exit(1);
        }
        fclose(fp);
    }
    return is_interesting + new_coverage;
}

int main(int argc, char* argv[])
{
    if (argc != 4 && argc != 5) {
        printf("Usage: %s <bitmap_source> <bitmap_dest> <bitmap_size> [<report_file>]\n", argv[0]);
        exit(-1);
    }

    // printf("%s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);

    int bitmap_size = strtol(argv[3], NULL, 10);
    if (bitmap_size > BITMAP_SIZE) {
        printf("Unexpected bitmap size. Fix me!");
        exit(-1);
    }

    load_bitmap(argv[1], bitmap_src, bitmap_size);
    load_bitmap(argv[2], bitmap_dst, bitmap_size);
    int r = compare_bitmaps(bitmap_src, bitmap_dst, argv[4], bitmap_size);
    if (r) save_bitmap(argv[2], bitmap_dst, bitmap_size);
    return r;
}