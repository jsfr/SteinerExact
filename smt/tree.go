package smt

import "math"

// Tree is defined as the following: It contains all N regular points, as the
// first N entries, and all N-2 Steiner points, as the N+1..2N-2 entries. The
// topology is then defined by the edges. adjacencies contains the index of all
// points that are neighbors to the Steiner point i, thus indices in
// adjacencies are sIdx-t.n
type Tree struct {
	n           int
	dim         int
	points      Points
	edges       Edges
	adjacencies [][3]int
}

// N is a getter for t.n
func (t *Tree) N() int {
	return t.n
}

// Dim is a getter for t.dim
func (t *Tree) Dim() int {
	return t.dim
}

// Points is a getter for t.points. This creates a copy of the list of points to
// avoid outside meddling.
func (t *Tree) Points() Points {
	points := make(Points, len(t.points))
	n := copy(points, t.points)
	if n != len(t.points) {
		panic("Not all points were copied.")
	}
	return points
}

// Edges is a getter for t.edges. This creates a copy of the list of edges to
// avoid outside meddling.
func (t *Tree) Edges() Edges {
	edges := make(Edges, len(t.edges))
	n := copy(edges, t.edges)
	if n != len(t.edges) {
		panic("Not all edges were copied.")
	}
	return edges
}

// Terminals works like Points, but only returns the terminal points.
func (t *Tree) Terminals() Points {
	terminals := t.points[:t.n]
	points := make(Points, len(terminals))
	n := copy(points, terminals)
	if n != len(terminals) {
		panic("Not all terminals were copied.")
	}
	return points
}

// SteinerPoints works like Points, but only returns the Steiner points.
func (t *Tree) SteinerPoints() Points {
	steiner := t.points[t.n:]
	points := make(Points, len(steiner))
	n := copy(points, steiner)
	if n != len(steiner) {
		panic("Not all Steiner points were copied.")
	}
	return points
}

// InitTree initializes the tree represented by the null-vector the given points
// must contain at least 3 points, and all given points are considered as
// regular points. Thus the number of regular points N is set to the number of
// points passed as an argument.
func InitTree(points *Points) *Tree {
	var t Tree
	t.n = len(*points)
	if t.n < 3 {
		panic("Too few points to initialize.")
	}

	// we use the first point as the decided dimension
	// all other points should have the same dimension
	t.dim = len((*points)[0])
	for _, p := range *points {
		if len(p) != t.dim {
			panic("All terminals are not of the same dimension.")
		}
	}

	t.points = make(Points, 0, 2*t.n-2)
	t.points = append(t.points, *points...)
	s := pertubedCentroid(0, 1, 2, &t)
	t.points = append(t.points, s)

	e0 := InitEdge(&t, 0, t.n)
	e1 := InitEdge(&t, 1, t.n)
	e2 := InitEdge(&t, 2, t.n)

	// there are n+k-1 edges = n+(n-2)-1 = 2n-3
	t.edges = make(Edges, 0, 2*t.n-3)
	t.edges = append(t.edges, e0, e1, e2)

	t.adjacencies = make([][3]int, t.n-2)
	t.adjacencies[0] = [3]int{0, 1, 2}

	return &t
}

// Sprout grows the tree by the next unconnected terminal. The connection is
// made on the edge at t.edges[edgeIdx]. After the sprouting two new edges are
// placed at the back of the edge list.
func (t *Tree) Sprout(edgeIdx int) {
	if len(t.points) >= 2*t.n-2 {
		panic("A FST cannot contain any more Steiner points")
	}

	// Select the terminals
	p0 := t.edges[edgeIdx].P0()
	p1 := t.edges[edgeIdx].P1()
	p2 := 2 + len(t.points) - t.n // The next terminal we need to connect

	// Get the new Steiner point and its number,
	// and append it to the point list
	s := pertubedCentroid(p0, p1, p2, t)
	sIdx := len(t.points)
	t.points = append(t.points, s)

	// Create the new edges and append them to the edge list
	e0 := InitEdge(t, p1, sIdx)
	e1 := InitEdge(t, p2, sIdx)
	t.edges = append(t.edges, e0)
	t.edges = append(t.edges, e1)

	// Change the end points of the original edge
	e2 := &t.edges[edgeIdx] // Should be defined AFTER append is used
	e2.UpdateEdge(p0, sIdx)

	// assign new adjacencies
	t.adjacencies[sIdx-t.n] = [3]int{p2, p0, p1}

	// update adjacencies if any of the other points were Steiner points
	for i := 0; i < 3; i++ {
		if p0 >= t.n && t.adjacencies[p0-t.n][i] == p1 {
			t.adjacencies[p0-t.n][i] = sIdx
		}
		if p1 >= t.n && t.adjacencies[p1-t.n][i] == p0 {
			t.adjacencies[p1-t.n][i] = sIdx
		}
	}
}

// Restore undoes the work done by Sprout. The function assumes that the edge
// t.edges[edgeIdx] is the latest sprouted.
func (t *Tree) Restore(edgeIdx int) {
	if len(t.edges) < 5 {
		panic("There are not enough edges to remove two and keep the original three")
	}

	idx := len(t.edges)

	// Get the edges we need to remove (e0, e1)
	// and the edge we need to restore (e2)
	e0 := t.edges[idx-2]
	e1 := t.edges[idx-1]
	e2 := &t.edges[edgeIdx]

	s := e2.P1()
	p0 := e2.P0()
	p1 := e0.P0()

	if s != e0.P1() || s != e1.P1() {
		panic("The edges do not go to the same Steiner point")
	}

	// Restore edge
	e2.UpdateEdge(p0, p1)

	// Remove Steiner point and last two edges
	t.points = t.points[:len(t.points)-1]
	t.edges = t.edges[:idx-2]

	// Delete adjacencies for the removed Steiner points
	t.adjacencies[s-t.n][0] = 0
	t.adjacencies[s-t.n][2] = 0
	t.adjacencies[s-t.n][1] = 0

	// Update adjacencies if any of the other points are Steiner points
	for i := 0; i < 3; i++ {
		if p0 >= t.n && t.adjacencies[p0-t.n][i] == s {
			t.adjacencies[p0-t.n][i] = p1
		}
		if p1 >= t.n && t.adjacencies[p1-t.n][i] == s {
			t.adjacencies[p1-t.n][i] = p0
		}
	}
}

// Error calculates the error of the tree. The error is as described in the
// article by Smith. Only angles greater than 120 degrees should affect it.
func (t *Tree) Error() float64 {
	pos := func(x float64) float64 {
		if x > 0 {
			return x
		}
		return 0
	}

	var error float64
	n := len(t.points) - t.n
	for i := 0; i < n; i++ {
		adj := t.adjacencies[i]
		sIdx := i + t.n

		s := t.points[sIdx]
		p0 := t.points[adj[0]]
		p1 := t.points[adj[1]]
		p2 := t.points[adj[2]]

		l0 := math.Sqrt(squaredDistance(s, p0))
		l1 := math.Sqrt(squaredDistance(s, p1))
		l2 := math.Sqrt(squaredDistance(s, p2))

		l01 := l0 * l1
		l12 := l1 * l2
		l20 := l2 * l0

		var p01, p12, p20 float64

		for i := 0; i < t.dim; i++ {
			v0 := p0[i] - s[i]
			v1 := p1[i] - s[i]
			v2 := p2[i] - s[i]
			p01 = p01 + v0*v1
			p12 = p12 + v1*v2
			p20 = p20 + v2*v0
		}

		error = error +
			pos(2*p01+l01) +
			pos(2*p12+l12) +
			pos(2*p20+l20)
	}
	return math.Sqrt(error)
}

// Length calculates the length of the entire tree by summing the length of all
// its edges.
func (t *Tree) Length() float64 {
	var length float64
	for _, e := range t.edges {
		length = length + e.Length()
	}
	return length
}
