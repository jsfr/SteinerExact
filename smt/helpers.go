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

/**
 * Analytically finds the Fermat-Torricelli point of the three specified points.
 */
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

func PrintTree(w *bufio.Writer, t *Tree, topvec []int) {
	fmt.Fprint(w,
		"=============== BEGIN TREE ===============",
		"\n### Topology Vector ###\n",
		topvec,
		"\n\n### Edges ###\n")
	for i, e := range t.edges {
		fmt.Fprintf(w, "(%v, %v), ", e.P0(), e.P1())
		if (i+1)%5 == 0 {
			fmt.Fprint(w, "\n")
		}
	}
	fmt.Fprint(w, "\n\n### Steiner points ###\n")
	for i, p := range t.points[t.n:] {
		fmt.Fprintln(w, t.n+i, ":", p)
	}
	fmt.Fprint(w, "\n### Error ###\t\t### Length ###\n",
		t.Error(), "\t", t.Length(),
		"\n=============== END TREE ===============\n\n")
	w.Flush()
}

func DebugTree(w *bufio.Writer, t *Tree) {
	fmt.Fprint(w,
		"###### BEGIN TREE ######",
		"\n### Edges ###\n")
	for i, e := range t.edges {
		e.SetLength()
		fmt.Fprintln(w, i, ":", e)
	}
	fmt.Fprint(w, "\n\n### Terminals ###\n")
	for i, p := range t.points[:t.n] {
		fmt.Fprintln(w, i, ":", p)
	}
	fmt.Fprint(w, "\n### Steiner points ###\n")
	for i, p := range t.points[t.n:] {
		fmt.Fprintln(w, t.n+i, ":", p)
	}
	fmt.Fprint(w, "\n### Adjacencies ###\n")
	for i, a := range t.adjacencies {
		fmt.Fprintln(w, i, ":", *a)
	}
	fmt.Fprint(w, "\n### Error ###\n", t.Error())
	fmt.Fprint(w,
		"\n\n### Length ###\n", t.Length(),
		"\n###### END TREE ######\n\n")
	w.Flush()
}
