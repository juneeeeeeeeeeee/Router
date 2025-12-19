#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define MAX_COST 2147483647
typedef struct _cell{
    int cell_cost;
    int pred;
    int path_cost;
    int length; // length from source to this cell. used for backtracking
    int layer;
    int x;
    int y;
    bool checked;
} cell;
typedef struct _net{
    int source_layer;
    int source_x;
    int source_y;
    int target_layer;
    int target_x;
    int target_y;
} net;
typedef struct _heap{
    int size;
    cell* arr;
} heap;

void heap_push(heap* hp, cell c) {
    int finding_pos = ++hp->size;
    while (finding_pos != 1 && hp->arr[finding_pos >> 1].path_cost > c.path_cost) {
        hp->arr[finding_pos] = hp->arr[finding_pos >> 1];
        finding_pos >>= 1;
    }
    hp->arr[finding_pos] = c;
}

cell heap_pop(heap* hp) {
    cell ret = hp->arr[1];
    cell last = hp->arr[hp->size--];
    int child = 2;
    while (child <= hp->size) {
        if (child < hp->size && hp->arr[child].path_cost > hp->arr[child + 1].path_cost) child++;
        if (last.path_cost < hp->arr[child].path_cost) break;
        hp->arr[child >> 1] = hp->arr[child];
        child <<= 1;
    }
    hp->arr[child >> 1] = last;
    return ret;
}

net* nets;
cell*** map;
// UP, NORTH, EAST, WEST, SOUTH, DOWN
int dx[6] = {0, 0, 1, -1, 0, 0};
int dy[6] = {0, 1, 0, 0, -1, 0};
int dz[6] = {1, 0, 0, 0, 0, -1};

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ./router <grid file> <output file>\n");
        return -1;
    }
    int x_size, y_size; // z_size is fixed to 2
    int bend_cost, via_cost;
    FILE* inputFile1 = fopen(argv[1], "r");
    if (inputFile1 == NULL) {
        fprintf(stderr, "Error opening file: %s\n", argv[1]);
        return -1;
    }
    fscanf(inputFile1, "%d %d %d %d", &x_size, &y_size, &bend_cost, &via_cost);
    map = (cell***)malloc(3 * sizeof(cell**)); // layer number is not zero-indexed
    for (int z = 1; z <= 2; z++) {
        map[z] = (cell**)malloc(y_size * sizeof(cell*));
        for (int y = 0; y < y_size; y++) {
            map[z][y] = (cell*)malloc(x_size * sizeof(cell));
            for (int x = 0; x < x_size; x++) {
                fscanf(inputFile1, "%d", &map[z][y][x].cell_cost);
                map[z][y][x].layer = z;
                map[z][y][x].x = x;
                map[z][y][x].y = y;
            }
        }
    }
    fclose(inputFile1);
    int net_count;
    FILE* inputFile2 = fopen(argv[2], "r");
    if (inputFile2 == NULL) {
        fprintf(stderr, "Error opening file: %s\n", argv[2]);
        return -1;
    }
    fscanf(inputFile2, "%d", &net_count);
    nets = (net*)malloc((net_count + 1) * sizeof(net));
    for (int i = 1; i <= net_count; i++) {
        fscanf(inputFile2, "%*d %d %d %d %d %d %d", &nets[i].source_layer, &nets[i].source_x, &nets[i].source_y,
               &nets[i].target_layer, &nets[i].target_x, &nets[i].target_y);
        map[nets[i].source_layer][nets[i].source_y][nets[i].source_x].cell_cost = -1; // to block other nets from using the source cell
        map[nets[i].target_layer][nets[i].target_y][nets[i].target_x].cell_cost = -1; // to block other nets from using the target cell
        map[nets[i].source_layer][nets[i].source_y][nets[i].source_x].length = 1;
    }
    fclose(inputFile2);

    printf("um\n");
    // Routing using Dijkstra's algorithm
    for (int n = 1; n <= net_count; n++) {
        // reset map
        for (int z = 1; z <= 2; z++) {
            for (int y = 0; y < y_size; y++) {
                for (int x = 0; x < x_size; x++) {
                    map[z][y][x].pred = -1;
                    map[z][y][x].path_cost = MAX_COST;
                    map[z][y][x].checked = false;
                }
            }
        }
        bool reached = false;
        heap hp;
        hp.size = 0;
        hp.arr = (cell*)malloc((x_size * y_size * 2 + 1) * sizeof(cell));
        map[nets[n].source_layer][nets[n].source_y][nets[n].source_x].cell_cost = 1; // ensure starting cell is not an obstacle
        map[nets[n].target_layer][nets[n].target_y][nets[n].target_x].cell_cost = 1; // ensure target cell is not an obstacle
        map[nets[n].source_layer][nets[n].source_y][nets[n].source_x].path_cost = 1;
        map[nets[n].source_layer][nets[n].source_y][nets[n].source_x].checked = true;
        heap_push(&hp, map[nets[n].source_layer][nets[n].source_y][nets[n].source_x]);
        while(hp.size) {
            cell current = heap_pop(&hp);
            if (current.layer == nets[n].target_layer && current.x == nets[n].target_x && current.y == nets[n].target_y) { // reached target
                reached = true;
                // cell* reversed_path = (cell*)malloc((current.length + 1) * sizeof(cell));

                // do backtracking and cleanup
                cell jun = current;
                cell um = map[current.layer + dz[current.pred]][current.y + dy[current.pred]][current.x + dx[current.pred]];
                printf("%d %d %d\n", jun.layer, jun.x, jun.y);
                while (um.layer != nets[n].source_layer || um.x != nets[n].source_x || um.y != nets[n].source_y) {
                    map[jun.layer][jun.y][jun.x].cell_cost = -1; // mark as obstacle
                    if (um.layer != jun.layer) { // via
                        printf("3 %d %d\n", um.x, um.y);
                    }
                    else {
                        printf("%d %d %d\n", um.layer, um.x, um.y);
                    }
                    jun = um;
                    um = map[um.layer + dz[um.pred]][um.y + dy[um.pred]][um.x + dx[um.pred]];
                }
                map[um.layer][um.y][um.x].cell_cost = -1; // mark as obstacle
                if (um.layer != jun.layer) { // via
                    printf("3 %d %d\n", um.x, um.y);
                }
                else {
                    printf("%d %d %d\n", um.layer, um.x, um.y);
                }

                printf("Reached target (%d, %d, %d) with path cost %d\n", current.layer, current.x, current.y, current.path_cost);
                break;
            }
            printf("Visiting cell (%d, %d, %d) with path cost %d\n", current.layer, current.x, current.y, current.path_cost);
            for (int dir = 0; dir < 6; dir++) {
                int nx = current.x + dx[dir];
                int ny = current.y + dy[dir];
                int nl = current.layer + dz[dir];
                if (nx < 0 || nx >= x_size || ny < 0 || ny >= y_size || nl < 1 || nl >= 3) continue;
                if (map[nl][ny][nx].cell_cost == -1) continue; // obstacle
                if (map[nl][ny][nx].checked) continue; // already checked

                map[nl][ny][nx].checked = true;
                map[nl][ny][nx].path_cost = map[current.layer][current.y][current.x].path_cost + map[nl][ny][nx].cell_cost;
                map[nl][ny][nx].length = map[current.layer][current.y][current.x].length + 1;
                map[nl][ny][nx].pred = 5 - dir;
                if (dir == 0 || dir == 5) map[nl][ny][nx].path_cost += via_cost;
                // else if (map[current.layer][current.y][current.x].pred != 5 - dir) map[nl][ny][nx].path_cost += bend_cost;
                heap_push(&hp, map[nl][ny][nx]);
            }
        }
        free(hp.arr);
    }
    // debugging
    printf("%d %d\n", map[nets[0].target_layer][nets[0].target_y][nets[0].target_x].path_cost, map[nets[0].target_layer][nets[0].target_y][nets[0].target_x].pred);
    for (int z = 0; z < 2; z++) {
        for (int y = 0; y < y_size; y++) free(map[z][y]);
        free(map[z]);
    }
    free(map);
    free(nets);
    return 0;
}