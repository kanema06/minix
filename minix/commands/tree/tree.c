#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int files_count = 0;
int dir_count = 0;

int alpha_sort(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}

void rtree(const char *path, int depth, int *last) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "tree: cannot open '%s': %s\n", path, strerror(errno));
        return;
    }

    struct dirent **entries;
    int n = scandir(path, &entries, NULL, alpha_sort);
    if (n < 0) {
        closedir(dir);
        fprintf(stderr, "tree: scandir failed '%s': %s\n", path, strerror(errno));
        return;
    }

    for (int i = 0; i < n; i++) {
        struct dirent *entry = entries[i];
        if (entry->d_name[0] == '.') {
            free(entry);
            continue;
        }

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        int is_last = (i == n - 1);

        for (int j = 0; j < depth; j++) {
            if (last[j])
                printf("    ");
            else
                printf("|   ");
        }
        printf("%s-- %s\n", is_last ? "`" : "|", entry->d_name);

        struct stat st;
        if (lstat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            dir_count++;
            last[depth] = is_last; 
            rtree(fullpath, depth + 1, last);
        } else {
            files_count++;
        }

        free(entry);
    }
    free(entries);
    closedir(dir);
}

void tree(const char *path) {
    files_count = 0;
    dir_count = 0;

    printf("%s\n", path);
    int last[1024] = {0};
    rtree(path, 0, last);

    printf("\n%d directory%s, %d file%s\n",
           dir_count + 1, (dir_count + 1 == 1) ? "" : "s",
           files_count, (files_count == 1) ? "" : "s");
}

int main(int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : ".";
    tree(path);
    return 0;
}