package smt

import (
	"bufio"
	"fmt"
	"math/rand"
)

func PertubedCentroid(idx0, idx1, idx2 int, t *Tree) (centroid Point) {
	p0 := t.points[idx0]
	p1 := t.points[idx1]
	p2 := t.points[idx2]
	centroid = make(Point, len(p0))
	for i := range centroid {
		centroid[i] = (p0[i]+p1[i]+p2[i])/3.0 +
			0.001*rand.Float64()
	}
	return
}

func FermatTorricelliPoint(idx [3]int, t *Tree) Point {
	// TODO
	return nil
}

func AdjacentPoints(sIdx int, t *Tree) (pIdx [3]int) {
	pIdx = [3]int{}
	for i, j := range t.adjacencies[sIdx] {
		e := t.edges[j]
		if e.P0() != sIdx {
			pIdx[i] = e.P0()
		} else {
			pIdx[i] = e.P1()
		}
	}
	return
}

func PrintTree(w *bufio.Writer, t *Tree, topvec []int, offset int) {
	fmt.Fprint(w,
		"=============== BEGIN TREE ===============",
		"\n### Topology Vector ###\n[")
	for _, x := range topvec {
		fmt.Fprintf(w, "%v ", x+offset)
	}
	fmt.Fprint(w, "\b]\n\n### Edges ###\n")
	for i, e := range t.edges {
		fmt.Fprintf(w, "(%2v %2v)", e.p0+offset, e.p1+offset)
		if i+1 == len(t.edges) {
			fmt.Fprint(w, "\n\n")
		} else {
			fmt.Fprint(w, ", ")
		}
		if (i+1)%4 == 0 {
			fmt.Fprint(w, "\n")
		}
	}
	fmt.Fprint(w, "\n### Steiner points ###\n")
	for i, p := range t.points[t.n:] {
		fmt.Fprintf(w, "%v : [", t.n+i+offset)
		for _, x := range p {
			fmt.Fprintf(w, "%10.10f ", x)
		}
		fmt.Fprint(w, "\b]\n")
	}
	fmt.Fprintf(w, "\n### Error ###\t\t### Length ###\n"+
		"%10.10f\t\t%10.10f"+
		"\n=============== END TREE ===============\n\n",
		t.Error(), t.Length())
	w.Flush()
}
