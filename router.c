#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define MAX_INT 2147483647
#define epsilon 1e-6
typedef struct _cell{
    int cell_cost;
    int pred;
    int path_cost;
    double score; // path_cost or estimated path cost
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
    int source_cell_cost; // to block other nets from using the source cell
    int target_layer;
    int target_x;
    int target_y;
    int target_cell_cost; // to block other nets from using the target cell
} net;
typedef struct _heap{
    int size;
    cell* arr;
} heap;

inline int abs (int a) {
    return (a < 0) ? -a : a;
}

void heap_push(heap* hp, cell c) {
    int finding_pos = ++hp->size;
    while (finding_pos != 1 && hp->arr[finding_pos >> 1].score > c.score) {
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
        if (child < hp->size && hp->arr[child].score > hp->arr[child + 1].score) child++;
        if (last.score < hp->arr[child].score) break;
        hp->arr[child >> 1] = hp->arr[child];
        child <<= 1;
    }
    hp->arr[child >> 1] = last;
    return ret;
}

int cost_predictor(cell current, cell target, int bend_cost, int via_cost) {
    int x_diff = abs(current.x - target.x);
    int y_diff = abs(current.y - target.y);
    int layer_diff = abs(current.layer - target.layer);
    int est_cost = x_diff + y_diff + layer_diff * via_cost;
    if (layer_diff == 0 && x_diff != 0 && y_diff != 0) est_cost += bend_cost;
    return est_cost;
}

net* nets;
cell*** map;

// UP, NORTH, EAST, WEST, SOUTH, DOWN
int dx[6] = {0, 0, 1, -1, 0, 0};
int dy[6] = {0, 1, 0, 0, -1, 0};
int dz[6] = {1, 0, 0, 0, 0, -1};

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: ./router <grid file> <output file> or ./router <grid file> <output file> --p\n");
        return -1;
    }
    bool use_predictor = false;
    if (argc == 4 && strcmp(argv[3], "--p") == 0) use_predictor = true; // will be implemented later
    
    int bend_cost, via_cost;
    FILE* inputFile1 = fopen(argv[1], "r");
    if (inputFile1 == NULL) {
        fprintf(stderr, "Error opening file: %s\n", argv[1]);
        return -1;
    }

    int x_size, y_size; // z_size is fixed to 2
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
        nets[i].source_cell_cost = map[nets[i].source_layer][nets[i].source_y][nets[i].source_x].cell_cost;
        nets[i].target_cell_cost = map[nets[i].target_layer][nets[i].target_y][nets[i].target_x].cell_cost;
        map[nets[i].source_layer][nets[i].source_y][nets[i].source_x].cell_cost = -1; // to block other nets from using the source cell
        map[nets[i].target_layer][nets[i].target_y][nets[i].target_x].cell_cost = -1; // to block other nets from using the target cell
        map[nets[i].source_layer][nets[i].source_y][nets[i].source_x].length = 1;
    }
    fclose(inputFile2);

    FILE* outputFile = fopen("output.txt", "w");
    heap hp;
    hp.arr = (cell*)malloc((x_size * y_size * 2 + 1) * sizeof(cell));
    cell* reversed_path = (cell*)malloc((x_size * y_size * 2) * sizeof(cell)); // worst case length

    // Routing using Dijkstra's algorithm
    for (int n = 1; n <= net_count; n++) {
        // reset map
        for (int z = 1; z <= 2; z++) {
            for (int y = 0; y < y_size; y++) {
                for (int x = 0; x < x_size; x++) {
                    map[z][y][x].pred = -1;
                    map[z][y][x].path_cost = MAX_INT;
                    map[z][y][x].score = MAX_INT;
                    map[z][y][x].checked = false;
                }
            }
        }
        hp.size = 0;
        bool reached = false;
        
        map[nets[n].source_layer][nets[n].source_y][nets[n].source_x].cell_cost = nets[n].source_cell_cost; // ensure starting cell is not an obstacle
        map[nets[n].target_layer][nets[n].target_y][nets[n].target_x].cell_cost = nets[n].target_cell_cost; // ensure target cell is not an obstacle
        map[nets[n].source_layer][nets[n].source_y][nets[n].source_x].path_cost = nets[n].source_cell_cost;
        map[nets[n].source_layer][nets[n].source_y][nets[n].source_x].score = nets[n].source_cell_cost;
        map[nets[n].source_layer][nets[n].source_y][nets[n].source_x].checked = true;
        heap_push(&hp, map[nets[n].source_layer][nets[n].source_y][nets[n].source_x]);
        int length;

        while(hp.size) {

            cell current = heap_pop(&hp);
            map[current.layer][current.y][current.x].checked = true;

            if (current.layer == nets[n].target_layer && current.x == nets[n].target_x && current.y == nets[n].target_y) { // reached target
                reached = true;

                // do backtracking
                cell tracking = current;
                while (tracking.layer != nets[n].source_layer || tracking.x != nets[n].source_x || tracking.y != nets[n].source_y) {
                    reversed_path[tracking.length - 1] = tracking;
                    tracking = map[tracking.layer + dz[tracking.pred]][tracking.y + dy[tracking.pred]][tracking.x + dx[tracking.pred]];
                }
                reversed_path[0] = map[tracking.layer][tracking.y][tracking.x];
                length = current.length;
                
                // debugging
                printf("Cost: %d\n", current.path_cost);
                break;
            }
            for (int dir = 0; dir < 6; dir++) {
                int nx = current.x + dx[dir];
                int ny = current.y + dy[dir];
                int nl = current.layer + dz[dir];
                if (nx < 0 || nx >= x_size || ny < 0 || ny >= y_size || nl < 1 || nl >= 3) continue;
                if (map[nl][ny][nx].cell_cost == -1) continue; // obstacle
                if (map[nl][ny][nx].checked) continue; // already checked
                
                map[nl][ny][nx].path_cost = map[current.layer][current.y][current.x].path_cost + map[nl][ny][nx].cell_cost;
                map[nl][ny][nx].length = map[current.layer][current.y][current.x].length + 1;
                map[nl][ny][nx].pred = 5 - dir;
                if (dir == 0 || dir == 5) map[nl][ny][nx].path_cost += via_cost;
                else if (map[current.layer][current.y][current.x].pred != 0 && map[current.layer][current.y][current.x].pred != 5 && // not via
                map[current.layer][current.y][current.x].pred != -1 && // current is not source
                dir != 5 - map[current.layer][current.y][current.x].pred) // direction changed
                    map[nl][ny][nx].path_cost += bend_cost;
                map[nl][ny][nx].score = map[nl][ny][nx].path_cost;
                heap_push(&hp, map[nl][ny][nx]);
            }
        }
        fprintf(outputFile, "%d\n", n);
        if (reached == true) {
            for (int i = 0; i < length; i++) {
                fprintf(outputFile, "%d %d %d\n", reversed_path[i].layer, reversed_path[i].x, reversed_path[i].y);
                printf("%d %d %d: cost %d\n", reversed_path[i].layer, reversed_path[i].x, reversed_path[i].y, reversed_path[i].path_cost);
                if (i != length - 1 && reversed_path[i].layer != reversed_path[i + 1].layer) { // via
                    fprintf(outputFile, "3 %d %d\n", reversed_path[i].x, reversed_path[i].y);
                }
                // do cleanup
                map[reversed_path[i].layer][reversed_path[i].y][reversed_path[i].x].cell_cost = -1; // block this cell for other nets
            }
        }
        fprintf(outputFile, "0\n"); // end of net
    }
    fclose(outputFile);
    free(hp.arr);
    free(reversed_path);
    for (int z = 1; z <= 2; z++) {
        for (int y = 0; y < y_size; y++) free(map[z][y]);
        free(map[z]);
    }
    free(map);
    free(nets);
    return 0;
}