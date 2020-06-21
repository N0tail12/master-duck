#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jrb.h>
#include <dllist.h>

#define INT_MAX 2147483647

typedef struct {
    JRB id;
    JRB name;
    JRB routes;
    JRB route_id;
    int n_id;
    int n_route;
} map_t;

typedef struct {
    JRB vertices;
    JRB edges;
} graph_t;

// return 1 if already has in the map
int map_insert(map_t map, int id, char *name);
// return 1 if already has in the map
int map_insert_route(map_t *map, char *route);
int map_get_route_id(map_t map, char *route);
char *map_get_route_name(map_t map, int route_id);
char *id2name(map_t map, int id);
int name2id(map_t map, char *name);
void free_map(map_t map);

graph_t load_graph_from_file(const char *fn, map_t *map);
void add_vertice(graph_t g, int id, char *name);
JRB get_vertice(graph_t g, int id);
void add_edge(graph_t g, int v1, int v2, int route_id);
JRB get_edge(graph_t g, int v1, int v2);
void drop_graph(graph_t g);

Dllist find_route(graph_t g, int v1, int v2, int n);
void print_route(graph_t g, map_t map, Dllist path);
void graph_traverse(graph_t g);

int main() {
    map_t map;

    graph_t g = load_graph_from_file("buses.txt", &map);
    printf("loaded %d vertices\n", map.n_id);
    // graph_traverse(g);
    
    int v1, v2;
    printf("v1: ");
    scanf("%d", &v1);
    printf("v2: ");
    scanf("%d", &v2);
    Dllist path = find_route(g, v1, v2, map.n_id);
    print_route(g, map, path);

    free_dllist(path);
    drop_graph(g);
    free_map(map);
}

void update(int *best_path, int *cur_path, int n, int *best_bus, int n_bus) {
    if (n_bus < *best_bus) {
        *best_bus = n_bus;
        for (int i = 0; i < n; ++i) {
            best_path[i] = cur_path[i];
        }
    }
}

void tryi(graph_t g, Dllist path, int *best_path, int *cur_path, int n, int *best_bus, int i, int n_bus) {
    if (n_bus >= *best_bus) return;
    if (i == n) {
        update(best_path, cur_path, n, best_bus, n_bus);
    } else {
        JRB routes = jval_v(get_edge(g, jval_i(path->flink->val), jval_i(path->flink->flink->val))->val);
        JRB route;
        jrb_traverse(route, routes) {
            int route_id = jval_i(route->key);
            cur_path[i] = route_id;
            int inc = i == 0 || (i > 0 && cur_path[i - 1] != route_id) ? 1 : 0;
            tryi(g, path->flink, best_path, cur_path, n, best_bus, i + 1, n_bus + inc);
        }
    }
}

void print_route(graph_t g, map_t map, Dllist path) {
    int n = -1;
    for (Dllist i = dll_first(path); i != path; i = i->flink) ++n;
    int *best_path = malloc(n * sizeof(int));
    int *cur_path = malloc(n * sizeof(int));
    int best_bus = INT_MAX;

    tryi(g, path, best_path, cur_path, n, &best_bus, 0, 0);

    printf("From %s to %s\n", id2name(map, jval_i(path->flink->val)), id2name(map, jval_i(path->blink->val)));
    printf("%s -> ", id2name(map, jval_i(path->flink->val)));
    printf("%s -> ", map_get_route_name(map, best_path[0]));

    Dllist tmp = path->flink->flink;
    for (int i = 1; i < n; ++i, tmp = tmp->flink) {
        if (tmp->flink) printf("%s -> ", id2name(map, jval_i(tmp->val)));
        if (best_path[i] != best_path[i - 1]) {
            printf("%s -> ", map_get_route_name(map, best_path[i]));
        }
    }
    printf("%s\n", id2name(map, jval_i(path->blink->val)));

    free(best_path);
    free(cur_path);
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

Dllist find_route(graph_t g, int v1, int v2, int n) {
    int *pre = malloc(n * sizeof(int));
    int *dist = malloc(n * sizeof(int));

    if (_bfs(g, v1, v2, pre, dist, n) == 0) {
        printf("Can't go to %d from %d\n", v2, v1);
        return new_dllist();
    }

    Dllist path = new_dllist();
    int crawl = v2;
    dll_append(path, new_jval_i(v2));
    while (pre[crawl] != -1) {
        dll_append(path, new_jval_i(pre[crawl]));
        crawl = pre[crawl];
    }

    free(pre);
    free(dist);

    return path;
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
                printf("%d, ", jval_i(route->key));
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

graph_t load_graph_from_file(const char *fn, map_t *map) {
    FILE *f = fopen(fn, "r");
    if (!f) {
        printf("Can't open file '%s'\n", fn);
        exit(1);
    }

    map->id = make_jrb();
    map->name = make_jrb();
    map->routes = make_jrb();
    map->route_id = make_jrb();
    map->n_id = 0;
    map->n_route = 0;

    graph_t g = {0};
    g.vertices = make_jrb();
    g.edges = make_jrb();


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
                    int dup1 = map_insert(*map, map->n_id, v1);
                    if (!dup1) ++(map->n_id);
                    int dup2 = map_insert(*map, map->n_id, v2);
                    if (!dup2) ++(map->n_id);

                    int id_v1 = name2id(*map, v1);
                    int id_v2 = name2id(*map, v2);

                    add_vertice(g, id_v1, id2name(*map, id_v1));
                    add_vertice(g, id_v2, id2name(*map, id_v2));

                    int dupr = map_insert_route(map, route);
                    if (!dupr) ++(map->n_route);

                    int id_route = map_get_route_id(*map, route);
                    add_edge(g, id_v1, id_v2, id_route);
                    add_edge(g, id_v2, id_v1, id_route);
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

void add_edge(graph_t g, int v1, int v2, int route_id) {
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

        node = jrb_find_int(routes, route_id);
        if (node == NULL) {
            jrb_insert_int(routes, route_id, new_jval_i(v2));
        }
    } else {
        JRB routes = jval_v(v1v2->val);
        JRB node = jrb_find_int(routes, route_id);
        if (node == NULL) {
            jrb_insert_int(routes, route_id, new_jval_i(v2));
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
            jrb_free_tree(jval_v(v2->val));
        }
        jrb_free_tree(v1);
    }
    jrb_free_tree(g.edges);
}

int map_insert(map_t map, int id, char *name) {
    JRB node = jrb_find_str(map.name, name);
    if (node == NULL) {
        char *dup_name = strdup(name);
        jrb_insert_str(map.name, dup_name, new_jval_i(id));
        jrb_insert_int(map.id, id, new_jval_s(dup_name));
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

int map_insert_route(map_t *map, char *route) {
    JRB node = jrb_find_str(map->routes, route);
    if (node == NULL) {
        char *dup_route = strdup(route);
        jrb_insert_str(map->routes, dup_route, new_jval_i(map->n_route));
        jrb_insert_int(map->route_id, map->n_route, new_jval_s(dup_route));
        ++(map->n_route);
        return 0;
    }
    return 1;
}

int map_get_route_id(map_t map, char *route) {
    JRB node = jrb_find_str(map.routes, route);
    if (node == NULL) return -1;
    return jval_i(node->val);
}

char *map_get_route_name(map_t map, int route_id) {
    JRB node = jrb_find_int(map.route_id, route_id);
    if (node == NULL) return NULL;
    return jval_s(node->val);
}

void free_map(map_t map) {
    JRB node;
    jrb_traverse(node, map.id) {
        free(jval_v(node->val));
    }
    jrb_free_tree(map.id);
    jrb_free_tree(map.name);

    jrb_traverse(node, map.routes) {
        free(jval_v(node->key));
    }
    jrb_free_tree(map.routes);
}