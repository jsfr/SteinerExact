package main

// A single regular or Steiner point Dim[i]
// stores the coordinate of the i-th dimension
type Point struct {
	Dim []float64
}

// A single edge. EndPoints contain the indexes of the end points for the
// edge. Length should be the euclidean length of the edge.
type Edge struct {
	EndPoints [2]int
	Length    float64
}

// A tree. This contains all N regular points, as the first N entries, and all
// N-2 Steiner points, as the N+1..2N-2 entries, in Points. The topology is then
// defined by the edges in Edges
type Tree struct {
	Points []Point
	Edges  []Edge
}

func (t *Tree) Init() {
	// Init tree with null vector
}

func (t *Tree) Sprout(edgeIdx int, pointIdx int) {
	// Find out the current edge number
	edgeNo := len(Edges)

	// Select the edge we sprout on
	sproutEdge := t.Edges[edgeIdx]

	newEdge1 := Edge{}
	newEdge2
}

func (t *Tree) Restore() {
	// Removes a RPoint and any extra SPoints created
}

func main() {
}
