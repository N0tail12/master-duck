#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <ctype.h>

#include <jrb.h>
#include <dllist.h>

#define INFINITIVE_VALUE  10e6

typedef struct {
    JRB edges;
    JRB vertices;
} Graph;

typedef struct {
    int id;
    char *name;
} vertice_t;


Graph loadGraphFromFile(const char *fn, int *num_v);

void addVertex(Graph graph, int *id, char *name);
JRB getVertex(Graph g, char *name);
void addEdge(Graph graph, int v1, int v2, const char *route);
JRB getEdge(Graph graph, int v1, int v2);
int indegree(Graph graph, int v, int *output);
int outdegree(Graph graph, int v, int *output);
void dropGraph(Graph graph);
double shortestPath(Graph graph, int s, int t, int *path, int *length);


int vert_str_cmp_func(Jval j1, Jval j2);
int vert_int_cmp_func(Jval j1, Jval j2);
void graphTraverse(Graph g);

int main() {
    Graph g = loadGraphFromFile("buses.txt");

    int num_v = 0;
    graphTraverse(g, &num_v);
    
    // dropGraph(g);
}

int BFS(Graph g, int s, int e, int num_v) {

}

double shortestPath(Graph graph, int s, int t, int *path, int *length) {

}

int nextRoute(char *s) {
    int i = 0;
    while (s[i] && s[i] != '-' && s[i] != '\n') ++i;
    return i;
}

Graph loadGraphFromFile(const char *fn, int *num_v) {
    int id = 0;
    Graph g = {0};

    FILE *f = fopen(fn, "r");
    if (!f) {
        printf("Can't open file '%s'\n", fn);
        return g;
    }

    g.edges = make_jrb();
    g.vertices = make_jrb();

    int count = 0;
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
                    addVertex(g, &id, v1);
                    addVertex(g, &id, v2);

                    JRB jv1 = getVertex(g, v1);
                    JRB jv2 = getVertex(g, v2);

                    int id_v1 = ((vertice_t*)jval_v(jv1->key))->id;
                    int id_v2 = ((vertice_t*)jval_v(jv2->key))->id;

                    addEdge(g, id_v1, id_v2, route);
                    addEdge(g, id_v2, id_v1, route);
                }
            }
        }
    }

    *num_v = id;
    return g;
}

void graphTraverse(Graph g) {
    JRB node;
    jrb_traverse(node, g.vertices) {
        vertice_t *v = jval_v(node->key);
        printf("%d: %s\n", v->id, v->name);
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

int vert_str_cmp_func(Jval j1, Jval j2) {
    char *s1 = ((vertice_t*)jval_v(j1))->name;
    char *s2 = ((vertice_t*)jval_v(j2))->name;

    char l1, l2;

    while (*s1 && *s2) {
        l1 = tolower(*s1);
        l2 = tolower(*s2);

        if (l1 == l2) {
            ++s1, ++s2;
            continue;
        } else if (l1 > l2) return 1;
        else return -1;
    }

    return *s1 ? 1 : *s2 ? -1 : 0;
}

int vert_int_cmp_func(Jval j1, Jval j2) {
    int i1 = ((vertice_t*)jval_v(j1))->id;
    int i2 = ((vertice_t*)jval_v(j2))->id;

    return i1 - i2;
}

void addVertex(Graph g, int *id, char *name) {
    vertice_t vt = {*id, name};
    JRB node = jrb_find_gen(g.vertices, new_jval_v(&vt), vert_str_cmp_func);

    if (node == NULL) {
        vertice_t *v = malloc(sizeof(vertice_t));
        v->id = *id;
        v->name = strdup(name);

        jrb_insert_gen(g.vertices, new_jval_v(v), new_jval_i((*id)++), vert_str_cmp_func);
    }
}

JRB getVertex(Graph g, char *name) {
    vertice_t vt = {-1, name};
    return jrb_find_gen(g.vertices, new_jval_v(&vt), vert_str_cmp_func);
}


void addEdge(Graph graph, int v1, int v2, const char *route) {
    JRB v1v2 = getEdge(graph, v1, v2);
    if (v1v2 == NULL) {
        JRB tree, node = jrb_find_int(graph.edges, v1);
        if (node == NULL) {
            tree = make_jrb();
            jrb_insert_int(graph.edges, v1, new_jval_v(tree));
        } else {
            tree = jval_v(node->val);
        }

        node = jrb_find_int(tree, v2);
        JRB routes;
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

JRB getEdge(Graph graph, int v1, int v2) {
    JRB node = jrb_find_int(graph.edges, v1);
    if (node == NULL) return NULL;

    return jrb_find_int(jval_v(node->val), v2);
}