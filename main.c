#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jrb.h>
#include <dllist.h>

#define INT_MAX 2147483647

typedef struct {
    JRB id;
    JRB name;
} map_t;

typedef struct {
    JRB vertices;
    JRB edges;
} graph_t;

// return 1 if already has in the map
int map_insert(map_t map, int id, char *name);
char *id2name(map_t map, int id);
int name2id(map_t map, char *name);
void free_map(map_t map);

graph_t load_graph_from_file(const char *fn, map_t *map, int *n);
void add_vertice(graph_t g, int id, char *name);
JRB get_vertice(graph_t g, int id);
void add_edge(graph_t g, int v1, int v2, char *route);
JRB get_edge(graph_t g, int v1, int v2);
void drop_graph(graph_t g);

void find_route(graph_t g, int v1, int v2, int n);
void graph_traverse(graph_t g);

int main() {
    map_t map;
    int n;

    graph_t g = load_graph_from_file("buses.txt", &map, &n);
    printf("loaded %d vertices\n", n);
    // graph_traverse(g);
    
    int v1, v2;
    printf("v1: ");
    scanf("%d", &v1);
    printf("v2: ");
    scanf("%d", &v2);
    find_route(g, v1, v2, n);

    drop_graph(g);
    free_map(map);
}

int _bfs(graph_t g, int v1, int v2, int *pre, int *dist, int n) {
    Dllist queue = new_dllist();
    char *visited = calloc(n, sizeof(char));

    for (int i = 0; i < n; ++i) {
        dist[i] = INT_MAX;
        pre[i] = -1;
    }

    visited[v1] = 1;
    dist[v1] = 0;
    dll_append(queue, new_jval_i(v1));

    while (!dll_empty(queue)) {
        Dllist node = dll_first(queue);
        int id_u = jval_i(node->val);
        dll_delete_node(node);

        JRB v, tree_u = jrb_find_int(g.edges, id_u);
        tree_u = jval_v(tree_u->val);
        jrb_traverse(v, tree_u) {
            int id_v = jval_i(v->key);
            if (!visited[id_v]) {
                // printf("v1 -> v2: %d %d\n", id_u, id_v);
                visited[id_v] = 1;
                dist[id_v] = dist[id_u] + 1;
                pre[id_v] = id_u;

                dll_append(queue, new_jval_i(id_v));

                if (id_v == v2) {
                    free(visited);
                    free_dllist(queue);
                    return 1;
                }
            }
        }
    }

    free_dllist(queue);
    free(visited);
    return 0;
}

void find_route(graph_t g, int v1, int v2, int n) {
    int *pre = malloc(n * sizeof(int));
    int *dist = malloc(n * sizeof(int));

    if (_bfs(g, v1, v2, pre, dist, n) == 0) {
        printf("Can't go to %d from %d\n", v2, v1);
        return;
    }

    Dllist path = new_dllist();
    int crawl = v2;
    dll_append(path, new_jval_i(v2));
    while (pre[crawl] != -1) {
        dll_append(path, new_jval_i(pre[crawl]));
        crawl = pre[crawl];
    }

    printf("From %d to %d:\n", v1, v2);
    Dllist i;
    dll_rtraverse(i, path) {
        printf("%d -> ", jval_i(i->val));
    }

    free_dllist(path);
    free(pre);
    free(dist);
}

void graph_traverse(graph_t g) {
    JRB node = NULL;
    jrb_traverse(node, g.vertices) {
        printf("%d: %s\n", jval_i(node->key), jval_s(node->val));
    }

    jrb_traverse(node, g.edges) {
        JRB v2, v1 = jval_v(node->val);
        jrb_traverse(v2, v1) {
            printf("%d->%d: ", jval_i(node->key), jval_i(v2->key));
            JRB route, routes = jval_v(v2->val);
            jrb_traverse(route, routes) {
                printf("%s, ", jval_s(route->key));
            }
            printf("\n");
        }
    }
}

int nextRoute(char *s) {
    int i = 0;
    while (s[i] && s[i] != '-' && s[i] != '\n') ++i;
    return i;
}

graph_t load_graph_from_file(const char *fn, map_t *map, int *n) {
    FILE *f = fopen(fn, "r");
    if (!f) {
        printf("Can't open file '%s'\n", fn);
        exit(1);
    }

    map->id = make_jrb();
    map->name = make_jrb();

    graph_t g = {0};
    g.vertices = make_jrb();
    g.edges = make_jrb();

    *n = 0;

    char line[1000], route[100], v1[100], v2[100];
    while (fgets(line, 999, f)) {
        if (line[0] == '[') {
            line[strlen(line) - 1] = '\0';
            strcpy(route, line);
            fgets(line, 999, f);

            char *s = line; int count = 0;
            while (*s && *s != '\n') {
                int i = nextRoute(s);
                s[i] = '\0';
                strcpy(v1, s);
                s += i + 1;

                int j = nextRoute(s);
                char c = s[j];
                s[j] = '\0';
                strcpy(v2, s);
                s[j] = c;

                if (*v2 != '\0') {
                    int dup1 = map_insert(*map, *n, v1);
                    if (!dup1) ++(*n);
                    int dup2 = map_insert(*map, *n, v2);
                    if (!dup2) ++(*n);

                    int id_v1 = name2id(*map, v1);
                    int id_v2 = name2id(*map, v2);

                    add_vertice(g, id_v1, id2name(*map, id_v1));
                    add_vertice(g, id_v2, id2name(*map, id_v2));

                    add_edge(g, id_v1, id_v2, route);
                    add_edge(g, id_v2, id_v1, route);
                }
            }
        }
    }

    return g;
}

void add_vertice(graph_t g, int id, char *name) {
    JRB node = jrb_find_int(g.vertices, id);
    if (node == NULL) {
        jrb_insert_int(g.vertices, id, new_jval_s(name));
    }
}

JRB get_vertice(graph_t g, int id) {
    return jrb_find_int(g.vertices, id);
}

void add_edge(graph_t g, int v1, int v2, char *route) {
    JRB v1v2 = get_edge(g, v1, v2);
    if (v1v2 == NULL) {
        JRB routes, tree, node = jrb_find_int(g.edges, v1);
        if (node == NULL) {
            tree = make_jrb();
            jrb_insert_int(g.edges, v1, new_jval_v(tree));
        } else {
            tree = jval_v(node->val);
        }

        node = jrb_find_int(tree, v2);
        if (node == NULL) {
            routes = make_jrb();
            jrb_insert_int(tree, v2, new_jval_v(routes));
        } else {
            routes = jval_v(node->val);
        }

        node = jrb_find_str(routes, route);
        if (node == NULL) {
            jrb_insert_str(routes, strdup(route), new_jval_i(v2));
        }
    } else {
        JRB routes = jval_v(v1v2->val);
        JRB node = jrb_find_str(routes, route);
        if (node == NULL) {
            jrb_insert_str(routes, strdup(route), new_jval_i(v2));
        }
    }
}

JRB get_edge(graph_t g, int v1, int v2) {
    JRB node = jrb_find_int(g.edges, v1);
    if (node == NULL) return NULL;

    return jrb_find_int(jval_v(node->val), v2);
}

void drop_graph(graph_t g) {
    jrb_free_tree(g.vertices);

    JRB node;
    jrb_traverse(node, g.edges) {
        JRB v2, v1 = jval_v(node->val);
        jrb_traverse(v2, v1) {
            JRB route, routes = jval_v(v2->val);
            jrb_traverse(route, routes) {
                free(jval_v(route->key));
            }
            jrb_free_tree(routes);
        }
        jrb_free_tree(v1);
    }
    jrb_free_tree(g.edges);
}

int map_insert(map_t map, int id, char *name) {
    JRB node = jrb_find_str(map.name, name);
    if (node == NULL) {
        jrb_insert_str(map.name, strdup(name), new_jval_i(id));
        jrb_insert_int(map.id, id, new_jval_s(strdup(name)));
        return 0;
    }
    return 1;
}

char *id2name(map_t map, int id) {
    JRB node = jrb_find_int(map.id, id);
    if (node == NULL) return NULL;
    return jval_s(node->val);
}

int name2id(map_t map, char *name) {
    JRB node = jrb_find_str(map.name, name);
    if (node == NULL) return -1;
    return jval_i(node->val);
}

void free_map(map_t map) {
    JRB node;
    jrb_traverse(node, map.id) {
        free(jval_v(node->val));
    }
    jrb_free_tree(map.id);

    jrb_traverse(node, map.name) {
        free(jval_v(node->key));
    }
    jrb_free_tree(map.name);
}