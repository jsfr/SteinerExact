package smt

import "math"

// Edge represents an edge in a tree. p0 and p1 are the index of points, length
// the length of the edge in the tree, and t is a reference to the tree the edge
// belongs to.
type Edge struct {
	p0     int
	p1     int
	length float64
	t      *Tree
}

// Edges is just a list of edges
type Edges []Edge

// P0 returns the index of the first point.
func (e *Edge) P0() int {
	return e.p0
}

// P1 returns the index of the second point.
func (e *Edge) P1() int {
	return e.p1
}

// Length calculates the length of the edge if this is necessary. Otherwise it
// simple returns the already stored length.
func (e *Edge) Length() float64 {
	if e.length < 0 {
		e.SetLength()
	}
	return e.length
}

// InitEdge creates and returns a new edge. The length is not calculated on
// creation, but is put of until needed.
func InitEdge(t *Tree, p0, p1 int) Edge {
	e := Edge{p0, p1, 0, t}
	e.UnsetLength()
	return e
}

// UpdateEdge updates the points of the edge and unsets the edge length to
// ensure it will be recalculated.
func (e *Edge) UpdateEdge(p0, p1 int) {
	e.p0 = p0
	e.p1 = p1
	e.UnsetLength()
}

// UnsetLength ensures that the edge length is recalculated upon the next
// Length() call.
func (e *Edge) UnsetLength() {
	e.length = -1
}

// SetLength recalculates the edge length and sets it. This should normally not
// be called on its own, as that will in most cases be handled using Length()
func (e *Edge) SetLength() {
	p0 := e.t.points[e.p0]
	p1 := e.t.points[e.p1]
	dist := squaredDistance(p0, p1)
	e.length = math.Sqrt(dist)
}
