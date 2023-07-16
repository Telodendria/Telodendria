/*
 * Copyright (C) 2022-2023 Jordan Bancino <@jordan:bancino.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Graph.h>

#include <Memory.h>

#include <string.h>

struct Graph
{
    size_t n;
    Edge *matrix;
};

Graph *
GraphCreate(size_t n)
{
    Graph *g;

    if (!n)
    {
        return NULL;
    }

    g = Malloc(sizeof(Graph));
    if (!g)
    {
        return NULL;
    }

    g->n = n;

    g->matrix = Malloc((n * n) * sizeof(Edge));
    if (!g->matrix)
    {
        Free(g);
        return NULL;
    }

    memset(g->matrix, 0, (n * n) * sizeof(Edge));

    return g;
}

Graph *
GraphCreateWithEdges(size_t n, Edge * matrix)
{
    Graph *g = GraphCreate(n);

    if (!g)
    {
        return NULL;
    }

    memcpy(g->matrix, matrix, (n * n) * sizeof(Edge));

    return g;
}

void
GraphFree(Graph * g)
{
    if (!g)
    {
        return;
    }

    Free(g->matrix);
    Free(g);
}

Edge
GraphEdgeGet(Graph * g, Node n1, Node n2)
{
    if (n1 >= g->n || n2 >= g->n)
    {
        return -1;
    }

    return g->matrix[(g->n * n1) + n2];
}

Edge
GraphEdgeSet(Graph * g, Node n1, Node n2, Edge e)
{
    int oldVal;

    if (n1 >= g->n || n2 >= g->n)
    {
        return -1;
    }

    if (e < 0)
    {
        return -1;
    }

    oldVal = g->matrix[(g->n * n1) + n2];

    g->matrix[(g->n * n1) + n2] = e;

    return oldVal;
}

size_t
GraphCountNodes(Graph * g)
{
    return g ? g->n : 0;
}

Node *
GraphBreadthFirstSearch(Graph * G, Node s, size_t * n)
{
    Node *visited;
    Node *queue;
    Node *result;
    size_t queueSize;
    Node i;

    if (!G || !n)
    {
        return NULL;
    }

    *n = 0;

    result = Malloc(G->n * sizeof(Node));
    if (!result)
    {
        return NULL;
    }

    if (s >= G->n)
    {
        Free(result);
        return NULL;
    }

    visited = Malloc(G->n * sizeof(Node));
    memset(visited, 0, G->n * sizeof(Node));
    queue = Malloc(G->n * sizeof(Node));
    queueSize = 0;

    visited[s] = 1;

    queueSize++;
    queue[queueSize - 1] = s;

    while (queueSize)
    {
        s = queue[queueSize - 1];
        queueSize--;

        result[*n] = s;
        (*n)++;

        for (i = 0; i < G->n; i++)
        {
            if (GraphEdgeGet(G, s, i) && !visited[i])
            {
                visited[i] = 1;

                queueSize++;
                queue[queueSize - 1] = i;
            }
        }
    }

    Free(visited);
    Free(queue);

    return result;
}

static void
GraphDepthFirstSearchRecursive(Graph * G, Node s, Node * result, size_t * n,
                               Node * visited)
{
    size_t i;

    visited[s] = 1;

    result[*n] = s;
    (*n)++;

    for (i = 0; i < G->n; i++)
    {
        if (GraphEdgeGet(G, s, i) && !visited[i])
        {
            GraphDepthFirstSearchRecursive(G, i, result, n, visited);
        }
    }
}

Node *
GraphDepthFirstSearch(Graph * G, Node s, size_t * n)
{
    Node *visited;
    Node *result;

    if (!G || !n)
    {
        return NULL;
    }

    result = Malloc(G->n * sizeof(Node));
    if (!result)
    {
        return NULL;
    }

    *n = 0;

    if (s >= G->n)
    {
        Free(result);
        return NULL;
    }

    visited = Malloc(G->n * sizeof(Node));
    memset(visited, 0, G->n * sizeof(Node));

    GraphDepthFirstSearchRecursive(G, s, result, n, visited);

    Free(visited);

    return result;
}

static void
GraphTopologicalSortRecursive(Graph * G, Node s, Node * visited,
                              Node * stack, size_t * stackSize)
{
    size_t i;

    visited[s] = 1;

    for (i = 0; i < G->n; i++)
    {
        if (GraphEdgeGet(G, s, i) && !visited[i])
        {
            GraphTopologicalSortRecursive(G, i, visited, stack, stackSize);
        }
    }

    stack[*stackSize] = s;
    (*stackSize)++;
}

Node *
GraphTopologicalSort(Graph * G, size_t * n)
{
    Node *visited;
    Node *stack;
    Node *result;

    size_t i;
    size_t stackSize;

    if (!G || !n)
    {
        return NULL;
    }

    *n = 0;

    result = Malloc(G->n * sizeof(Node));
    if (!result)
    {
        return NULL;
    }

    visited = Malloc(G->n * sizeof(Node));
    memset(visited, 0, G->n * sizeof(Node));
    stack = Malloc(G->n * sizeof(Node));
    memset(stack, 0, G->n * sizeof(Node));

    stackSize = 0;

    for (i = 0; i < G->n; i++)
    {
        if (!visited[i])
        {
            GraphTopologicalSortRecursive(G, i, visited, stack, &stackSize);
        }
    }

    Free(visited);

    while (stackSize)
    {
        stackSize--;
        result[*n] = stack[stackSize];
        (*n)++;
    }

    Free(stack);

    return result;
}

Graph *
GraphTranspose(Graph * G)
{
    Graph *T = Malloc(sizeof(Graph));
    size_t i, j;

    T->n = G->n;
    T->matrix = Malloc((G->n * G->n) * sizeof(Edge));

    memset(T->matrix, 0, (T->n * T->n) * sizeof(Edge));

    for (i = 0; i < G->n; i++)
    {
        for (j = 0; j < G->n; j++)
        {
            if (GraphEdgeGet(G, i, j))
            {
                GraphEdgeSet(T, j, i, GraphEdgeGet(G, i, j));
            }
        }
    }

    return T;
}
