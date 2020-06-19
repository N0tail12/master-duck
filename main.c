
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jrb.h>

typedef struct {
    JRB id;
    JRB name;
} map_t;

typedef struct {
    JRB vertices;
    JRB edges;
} graph_t;

// return 1 if duplicated
int map_insert(map_t map, int *id, char *name);
int name2id(map_t map, char *name);
char *id2name(map_t map, int id);

graph_t load_graph_from_file(char *fn, map_t *map, int *num_v);
void add_vertice(graph_t g, int id, char *name);
JRB get_vertice(graph_t g, int id);
void add_edge(graph_t g, int v1, int v2, char *route);
JRB get_edge(graph_t g, int v1, int v2);

void graph_traverse(graph_t g);

int main() {
    map_t map;
    int num_v = 0;
    
    graph_t g = load_graph_from_file("buses.txt", &map, &num_v);
    printf("loaded %d vertices\n", num_v);
    graph_traverse(g);
}

void graph_traverse(graph_t g) {
    JRB node;
    jrb_traverse(node, g.vertices) {
        printf("%d: %s\n", jval_i(node->key), jval_s(node->val));
    }
}

int nextRoute(char *s) {
    int i = 0;
    while (s[i] && s[i] != '-' && s[i] != '\n') ++i;
    return i;
}

graph_t load_graph_from_file(char *fn, map_t *map, int *num_v) {
    graph_t g = {0};

    FILE *f = fopen(fn, "r");
    if (!f) {
        printf("Can't open file '%s'\n", fn);
        return g;
    }

    map->id = make_jrb();
    map->name = make_jrb();
    *num_v = 0;

    g.edges = make_jrb();
    g.vertices = make_jrb();

    char line[1000], route[100], v1[100], v2[100];
    while (fgets(line, 999, f)) {
        if (line[0] == '[') {
            line[strlen(line) - 1] = '\0';
            strcpy(route, line);
            fgets(line, 999, f);

            char *s = line;
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
                    map_insert(*map, num_v, v1);
                    map_insert(*map, num_v, v2);

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

    jrb_find_int(jval_v(node->val), v2);
}


int name2id(map_t map, char *name) {
    JRB node = jrb_find_str(map.name, name);
    if (node == NULL) return -1;
    else return jval_i(node->val);
}

char *id2name(map_t map, int id) {
    JRB node = jrb_find_int(map.id, id);
    if (node == NULL) return NULL;
    else return jval_s(node->val);
}

// return 1 if duplicate
int map_insert(map_t map, int *id, char *name) {
    JRB node = jrb_find_str(map.name, name);
    if (node == NULL) {
        jrb_insert_str(map.name, name, new_jval_i(*id));
        jrb_insert_int(map.id, *id, new_jval_s(strdup(name)));
        ++(*id);
        return 0;
    }
    return 1;
}