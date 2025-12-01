#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <omp.h>

typedef struct {
    double x, y;
} Point;

typedef struct {
    double xmin, ymin, xmax, ymax;
} Region;

typedef struct QuadtreeNode {
    Region boundary;
    Point *points;
    int count;
    int capacity;
    bool divided;
    struct QuadtreeNode *nw;
    struct QuadtreeNode *ne;
    struct QuadtreeNode *sw;
    struct QuadtreeNode *se;
} QuadtreeNode;

QuadtreeNode *quadtree_create(Region boundary, int capacity) {
    QuadtreeNode *node = malloc(sizeof(QuadtreeNode));
    node->boundary = boundary;
    node->capacity = capacity;
    node->points = malloc(sizeof(Point) * capacity);
    node->count = 0;
    node->divided = false;
    node->nw = node->ne = node->sw = node->se = NULL;
    return node;
}

bool region_contains(Region r, Point p) {
    return (p.x >= r.xmin && p.x <= r.xmax && p.y >= r.ymin && p.y <= r.ymax);
}

bool region_intersect(Region a, Region b) {
    return !(b.xmin > a.xmax || b.xmax < a.xmin || b.ymin > a.ymax || b.ymax < a.ymin);
}

void quadtree_subdivide(QuadtreeNode *node) {
    double xmid = (node->boundary.xmin + node->boundary.xmax) / 2.0;
    double ymid = (node->boundary.ymin + node->boundary.ymax) / 2.0;
    Region r_nw = { node->boundary.xmin, ymid, xmid, node->boundary.ymax };
    Region r_ne = { xmid, ymid, node->boundary.xmax, node->boundary.ymax };
    Region r_sw = { node->boundary.xmin, node->boundary.ymin, xmid, ymid };
    Region r_se = { xmid, node->boundary.ymin, node->boundary.xmax, ymid };
    node->nw = quadtree_create(r_nw, node->capacity);
    node->ne = quadtree_create(r_ne, node->capacity);
    node->sw = quadtree_create(r_sw, node->capacity);
    node->se = quadtree_create(r_se, node->capacity);
    node->divided = true;
}

void quadtree_insert(QuadtreeNode *node, Point p) {
    if (!region_contains(node->boundary, p)) return;
    if (node->count < node->capacity) {
        node->points[node->count++] = p;
    } else {
        if (!node->divided) quadtree_subdivide(node);
        quadtree_insert(node->nw, p);
        quadtree_insert(node->ne, p);
        quadtree_insert(node->sw, p);
        quadtree_insert(node->se, p);
    }
}

void quadtree_query(QuadtreeNode *node, Region range, Point *found, int *foundCount) {
    if (!region_intersect(node->boundary, range)) return;
    for (int i = 0; i < node->count; i++)
        if (region_contains(range, node->points[i])) found[(*foundCount)++] = node->points[i];
    if (node->divided) {
        quadtree_query(node->nw, range, found, foundCount);
        quadtree_query(node->ne, range, found, foundCount);
        quadtree_query(node->sw, range, found, foundCount);
        quadtree_query(node->se, range, found, foundCount);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <N> <CAP>\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);
    int capacity = atoi(argv[2]);
    Region boundary = {0.0, 0.0, 100.0, 100.0};
    QuadtreeNode *qt = quadtree_create(boundary, capacity);
    Point *points = malloc(sizeof(Point) * N);
    for (int i = 0; i < N; i++) {
        points[i].x = (double)rand() / RAND_MAX * 100.0;
        points[i].y = (double)rand() / RAND_MAX * 100.0;
    }
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < N; i++) {
        #pragma omp critical
        quadtree_insert(qt, points[i]);
    }
    Region query = {25.0, 25.0, 75.0, 75.0};
    Point *found = malloc(sizeof(Point) * N);
    int foundCount = 0;
    double start = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp single nowait
        quadtree_query(qt, query, found, &foundCount);
    }
    double end = omp_get_wtime();
    printf("Pontos encontrados: %d\n", foundCount);
    printf("Tempo: %.6f s\n", end - start);
    free(points);
    free(found);
    return 0;
}
