package smt

import "math"

// An edge is defined as the index of the two points defining is begin and end
// points in a tree. The edge should always follow the convention that P1 < P2.
type Edge struct {
	p0     int
	p1     int
	length float64
}

func (e *Edge) P0() int {
	return e.p0
}

func (e *Edge) P1() int {
	return e.p1
}

func (e *Edge) Length() float64 {
	return e.length
}

func InitEdge(t *Tree, p0, p1 int) Edge {
	e := Edge{p0, p1, 0}
	e.length = e.UpdateLength(t)
	return e
}

func (e *Edge) UpdateEdge(t *Tree, p0, p1 int) {
	e.p0 = p0
	e.p1 = p1
	e.length = e.UpdateLength(t)
}

func (e *Edge) UpdateLength(t *Tree) float64 {
	p0 := t.Points()[e.p0]
	p1 := t.Points()[e.p1]

	dist := 0.0
	for i := range p0 {
		diff := p0[i] - p1[i]
		dist = dist + (diff * diff)
	}
	dist = math.Sqrt(dist)
	return dist
}
