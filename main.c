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


Graph loadGraphFromFile(const char *fn);

void addVertex(Graph graph, int *id, char *name);
int getVertex(Graph graph, char *name);
void addEdge(Graph graph, int v1, int v2, const char *route);
Dllist getEdgeValue(Graph graph, int v1, int v2);
int indegree(Graph graph, int v, int *output);
int outdegree(Graph graph, int v, int *output);
void dropGraph(Graph graph);
double shortestPath(Graph graph, int s, int t, int *path, int *length);


int main() {
    Graph g = loadGraphFromFile("buses.txt");

    JRB node;
    jrb_traverse(node, g.vertices) {
        printf("%d: %s\n", jval_i(node->val), jval_s(node->key));
    }

    jrb_traverse(node, g.edges) {
        printf("%d: ", jval_i(node->key));
        JRB tree, edges = jval_v(node->val);
        jrb_traverse(tree, edges) {
            Dllist i, list = jval_v(tree->val);
            dll_traverse(i, list) {
                printf("%s, ", jval_s(i->val));
            }
        }
        printf("\n");
    }
    // dropGraph(g);
}

int nextRoute(char *s) {
    int i = 0;
    while (s[i] && s[i] != '-' && s[i] != '\n') ++i;
    return i;
}

Graph loadGraphFromFile(const char *fn) {
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

                    addEdge(g, getVertex(g, v1), getVertex(g, v2), route);
                    addEdge(g, getVertex(g, v2), getVertex(g, v1), route);
                }
            }
        }
    }

    return g;
}

int cmp_func(Jval j1, Jval j2) {
    char *s1 = jval_s(j1);
    char *s2 = jval_s(j2);

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

void addVertex(Graph g, int *id, char *name) {
    // JRB node = jrb_find_int(g.vertices, id);
    // if (node == NULL) // only add new vertex
    //     jrb_insert_int(g.vertices, id, new_jval_s(strdup(name)));

    JRB node = jrb_find_gen(g.vertices, new_jval_s(name), cmp_func);
    if (node == NULL) jrb_insert_gen(g.vertices, new_jval_s(strdup(name)), new_jval_i((*id)++), cmp_func);
}

int getVertex(Graph g, char *name) {
    JRB node = jrb_find_gen(g.vertices, new_jval_s(name), cmp_func);
    if (node == NULL) return -1;
    return jval_i(node->val);
}


void addEdge(Graph graph, int v1, int v2, const char *route) {
    JRB node, tree;
    Dllist routes = getEdgeValue(graph, v1, v2);
    if (routes == NULL) {
        node = jrb_find_int(graph.edges, v1);
        if (node == NULL) {
            tree = make_jrb();
            jrb_insert_int(graph.edges, v1, new_jval_v(tree));
        } else {
            tree = (JRB) jval_v(node->val);
        }
        routes = new_dllist();
        routes->val = new_jval_s(strdup(route));
        jrb_insert_int(tree, v2, new_jval_v(routes));
    } else {
        dll_append(routes, new_jval_s(strdup(route)));
    }
}

Dllist getEdgeValue(Graph graph, int v1, int v2) {
    JRB node, tree;
    node = jrb_find_int(graph.edges, v1);
    if (node == NULL)
        return NULL;
    tree = (JRB) jval_v(node->val);
    node = jrb_find_int(tree, v2);
    if (node == NULL)
        return NULL;
    else
        return jval_v(node->val);
}