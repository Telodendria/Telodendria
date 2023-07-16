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

#ifndef CYTOPLASM_GRAPH_H
#define CYTOPLASM_GRAPH_H

/***
 * @Nm Graph
 * @Nd Extremely simple graph, implemented as an adjacency matrix.
 * @Dd July 15 2023
 *
 * .Nm
 * is a basic graph data structure originally written for a computer
 * science class on data structures and algorithms, in which it
 * received full credit. This is an adaptation of the original
 * implementation that follows the Cytoplasm style and uses Cytoplasm
 * APIs when convenient.
 * .P
 * .Nm
 * stores data in an adjacency matrix, which means the storage
 * complexity is O(N^2), where N is the number of vertices (called
 * Nodes in this implementation) in the graph. However, this makes the
 * algorithms fast and efficient.
 * .P
 * Nodes are identified by index, so the first node is 0, the second
 * is 1, and so on. This data structure does not support storing
 * arbitrary data as nodes; rather, the intended use case is to add
 * all your node data to an Array, thus giving each node an index,
 * and then manipulating the graph with that index. This allows access
 * to node data in O(1) time in call cases, and is the most memory
 * efficient.
 * .P
 * .Nm
 * can be used to store a variety of types of graphs, although it is
 * primarily suited to directed and weighted graphs.
 */

#include <stddef.h>

/**
 * The functions provided here operate on an opaque graph structure.
 * This structure really just stores a matrix in a contiguous block of
 * memory, as well as the number of nodes in the graph, but the
 * structure is kept opaque so that it remains internally consistent.
 * It also maintains the style of the Cytoplasm library.
 */
typedef struct Graph Graph;

/**
 * An Edge is really just a weight, which is easily represented by an
 * integer. However, it makes sense to alias this to Edge for clarity,
 * both in the documentation and in the implementation.
 */
typedef int Edge;

/**
 * A Node is really just a row or column in the matrix, which is easily
 * represented by an unsigned integer. However, it makes sense to alias
 * this to Node for clarity, both in the documentation and the
 * implementation.
 */
typedef size_t Node;

/**
 * Create a new graph structure with the given number of vertices.
 */
extern Graph *GraphCreate(size_t);

/**
 * Create a new graph data structure with the given number of vertices
 * and the given adjacency matrix. The adjacency matrix is copied
 * verbatim into the graph data structure without any validation.
 */
extern Graph *GraphCreateWithEdges(size_t, Edge *);

/**
 * Free all memory associated with the given graph. Since graphs are
 * just a collection of numbers, they do not depend on each other in
 * any way.
 */
extern void GraphFree(Graph *);

/**
 * Get the weight of the edge connecting the node specified first to
 * the node specified second. If this is a directed graph, it does not
 * necessarily follow that there is an edge from the node specified
 * second to the node specified first. It also does not follow that
 * such an edge, if it exists, has the same weight.
 * .P
 * This function will return -1 if the graph is invalid or either node
 * is out of bounds. It will return 0 if there is no such edge from the
 * node specified first to the node specified second.
 */
extern Edge GraphEdgeGet(Graph *, Node, Node);

/**
 * Set the weight of the edge connecting the node specified first to
 * the node specified second. If this is not a directed graph, this
 * function will have to be called twice, the second time reversing the
 * order of the nodes. To remove the edge, specify a weight of 0.
 */
extern Edge GraphEdgeSet(Graph *, Node, Node, Edge);

/**
 * Get the number of nodes in the given graph. This operation is a
 * simple memory access that happens in O(1) time.
 */
extern size_t GraphCountNodes(Graph *);

/**
 * Perform a breadth-first search on the given graph, starting at the
 * specified node. This function returns a list of nodes in the order
 * they were touched. The size of the list is stored in the unsigned
 * integer pointed to by the last argument.
 * .P
 * If an error occurs, NULL will be returned. Otherwise, the returned
 * pointer should be freed with the Memory API when it is no longer
 * needed.
 */
extern Node * GraphBreadthFirstSearch(Graph *, Node, size_t *);

/**
 * Perform a depth-first search on the given graph, starting at the
 * specified node. This function returns a list of nodes in the order
 * they were touched. The size of the list is stored in the unsigned
 * integer pointed to by the last argument.
 * .P
 * If an error occurs, NULL will be returned. Otherwise the returned
 * pointer should be freed with the Memory API when it is no longer
 * needed.
 */
extern Node *GraphDepthFirstSearch(Graph *, Node, size_t *);

/**
 * Perform a topological sort on the given graph. This function returns
 * a list of nodes in topological ordering, though note that this is
 * probably not the only topological ordering that exists for the
 * graph. The size of the list is stored in the unsigned integer
 * pointed to by the last argument. It should always be the number of
 * nodes in the graph, but is provided for consistency and convenience.
 * .P
 * If an error occurs, NULL will be returned. Otherwise the returned
 * pointer should be freed with the Memory API when it is no longer
 * needed.
 */
extern Node *GraphTopologicalSort(Graph *, size_t *);

/**
 * Transpose the given graph, returning a brand new graph that is the
 * result of the transposition. 
 */
extern Graph * GraphTranspose(Graph *);

#endif /* CYTOPLASM_GRAPH_H */
