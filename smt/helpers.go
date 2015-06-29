package smt

import (
	"bufio"
	"fmt"
	"math"
	"math/rand"
)

func pertubedCentroid(idx0, idx1, idx2 int, t *Tree) (centroid Point) {
	p0 := t.points[idx0]
	p1 := t.points[idx1]
	p2 := t.points[idx2]
	centroid = make(Point, len(p0))
	for i := range centroid {
		centroid[i] = (p0[i]+p1[i]+p2[i])/3.0 +
			1e-3*rand.Float64()
	}
	return
}

func fermatTorricelliPoint(sIdx int, pIdx [3]int, t *Tree) Point {
	steinerPoint := make(Point, t.dim)

	p := make(Points, 3)
	for i := range pIdx {
		p[i] = t.points[pIdx[i]]
	}

	epsilon := 1e-4 * t.Error() / float64(t.n)
	a12 := squaredDistance(p[0], p[1])
	a23 := squaredDistance(p[1], p[2])
	a31 := squaredDistance(p[2], p[0])
	d12 := math.Sqrt(a12)
	d23 := math.Sqrt(a23)
	d31 := math.Sqrt(a31)

	switch {
	case a12+a31-a23+d12*d31 <= epsilon:
		//steiner point is at p[0]
		copy(steinerPoint, p[0])
	case a12+a23-a31+d12*d23 <= epsilon:
		//steiner point is at p[1]
		copy(steinerPoint, p[1])
	case a23+a31-a12+d23*d31 <= epsilon:
		//steiner point is at p[2]
		copy(steinerPoint, p[2])
	default:
		sqrt3over2 := math.Sqrt(3) / 2
		q := (d12 + d23 + d31) / 2
		s := 2 * math.Sqrt(q*(q-d12)*(q-d23)*(q-d31))
		k0 := (a12+a31-a23)*sqrt3over2 + s
		k1 := (a23+a12-a31)*sqrt3over2 + s
		k2 := (a31+a23-a12)*sqrt3over2 + s
		co := k0 * k1 * k2 / (2 * s * (k0 + k1 + k2))
		for i := range steinerPoint {
			steinerPoint[i] = co * (p[0][i]/k0 + p[1][i]/k1 + p[2][i]/k2)
		}
	}

	return steinerPoint
}

// PrintTree outputs the tree and topology vector is is given in a customized
// format. All indices outputted are offset by the given integer.
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
	fmt.Fprint(w, "\n### Terminals ###\n")
	for i, p := range t.points[:t.n] {
		fmt.Fprintf(w, "%v : [", i+offset)
		for _, x := range p {
			fmt.Fprintf(w, "%10.10f ", x)
		}
		fmt.Fprint(w, "\b]\n")
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

func squaredDistance(p0, p1 Point) float64 {
	dist := 0.0
	for i := range p0 {
		diff := p0[i] - p1[i]
		dist = dist + (diff * diff)
	}
	return dist
}
