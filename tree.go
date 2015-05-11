package main

// A tree. This contains all N regular points, as the first N entries, and all
// N-2 Steiner points, as the N+1..2N-2 entries. The topology is then defined by
// the edges.
type Tree struct {
	N      int
	Points []Point
	Edges  []Edge
}

// Initializes the tree represented by the null-vector the given points must
// contain at least 3 points, and all given points are considered as regular
// points. Thus the number of regular points N is set to the number of points
// passed as an argument.
func (t *Tree) Init(points []Point) {
	n := len(points)
	if n < 3 {
		panic("Too few points to initialize.")
	}

	s0 := GetPertubedCentroid(t.Points[0], t.Points[1], t.Points[2])

	e0 := Edge{1, n + 1, t}
	e1 := Edge{2, n + 1, t}
	e2 := Edge{3, n + 1, t}

	// there are n+k-1 edges = n+(n-2)-1 = 2n-3
	edges := make([]Edge, 3, 2*t.N-3)
	edges = []Edge{e0, e1, e2}

	t.N = n
	t.Points = append(t.Points, s0)
	t.Edges = edges
}

func (t *Tree) Sprout(edgeIdx, pointIdx) {
	edgeNo := len(t.Edges)
	steinerNo := len(t.Edges)

	// Select the edge we split. Use a pointer for easy editing
	sproutEdge := &t.Edges[edgeIdx]

	s := GetPertubedCentroid()

	newEdge1 := Edge{}
}

// func (t *Tree) Sprout(edgeIdx int, pointIdx int) {
// 	// Find out the current edge number
// 	//	edgeNo := len(t.Edges)

// 	// Select the edge we sprout on
// 	//	sproutEdge := &t.Edges[edgeIdx]

// 	// newEdge1 := Edge{[]int{}, 0.0}
// 	// newEdge2 := Edge{[]int{}, 0.0}
// }

// func (t *Tree) Restore() {
// 	// Removes a RPoint and any extra SPoints created
// }
