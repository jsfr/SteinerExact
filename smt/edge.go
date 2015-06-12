package smt

import "math"

// An edge is defined as the index of the two points defining is begin and end
// points in a tree. The edge should always follow the convention that P1 < P2.
type Edge struct {
	p0     int
	p1     int
	length float64
	t      *Tree
}

func (e *Edge) P0() int {
	return e.p0
}

func (e *Edge) P1() int {
	return e.p1
}

func (e *Edge) Length() float64 {
	if e.length < 0 {
		e.SetLength()
	}
	return e.length
}

func InitEdge(t *Tree, p0, p1 int) Edge {
	e := Edge{p0, p1, 0, t}
	e.UnsetLength()
	return e
}

func (e *Edge) UpdateEdge(p0, p1 int) {
	e.p0 = p0
	e.p1 = p1
	e.UnsetLength()
}

/**
 * This function forces a recalculation of the length
 * the next time e.Length() is called
 */
func (e *Edge) UnsetLength() {
	e.length = -1
}

/**
 * Recalculates the edge length and sets it.
 * This should normally not be called
 * on its own, as that will in most cases be handled
 * using e.Length()
 */
func (e *Edge) SetLength() {
	p0 := e.t.points[e.p0]
	p1 := e.t.points[e.p1]
	dist := 0.0
	for i := range p0 {
		diff := p0[i] - p1[i]
		dist = dist + (diff * diff)
	}
	e.length = math.Sqrt(dist)
}
