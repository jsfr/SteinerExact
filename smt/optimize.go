package smt

import "math"

// SmithsIteration runs a single of iteration as proposed by Smith.
// TODO: This implementation is more or less shamelessly copied from
// Smiths implementation. If there is time, it would be nice to try implementing
// the gaussian elimination better, and possibly using goroutines. E.g. we could
// have a goroutine for each dimension as these are actually separate.
func (t *Tree) SmithsIteration(epsilon float64) {
	// Set up equations
	// t.points[t.n+i][] = sum_{j}( B[i][j] * t.points[j][] ) + C[i]

	B := make([][3]float64, len(t.points))
	C := make([][]float64, len(t.points))
	val := make([]int, len(t.points)-t.n)
	leafQ := make(stack, 0, len(t.points))
	eqnStack := make(stack, 0, len(t.points))
	n := len(t.points) - t.n

	// Prepare equations
	for i := 0; i < n; i++ {
		adj := t.adjacencies[i]
		sIdx := i + t.n
		s := t.points[sIdx]
		p0 := t.points[adj[0]]
		p1 := t.points[adj[1]]
		p2 := t.points[adj[2]]

		q := [3]float64{
			1 / (math.Sqrt(squaredDistance(s, p0)) + epsilon),
			1 / (math.Sqrt(squaredDistance(s, p1)) + epsilon),
			1 / (math.Sqrt(squaredDistance(s, p2)) + epsilon),
		}
		sum := q[0] + q[1] + q[2]
		q[0] = q[0] / sum
		q[1] = q[1] / sum
		q[2] = q[2] / sum

		C[sIdx] = make([]float64, t.dim)
		for a := 0; a < 3; a++ {
			b := adj[a]
			c := q[a]
			i := sIdx
			if b >= t.n {
				val[i-t.n]++
				B[i][a] = c
			} else {
				for m := range C[i] {
					C[i][m] = C[i][m] + t.points[b][m]*c
				}
			}
		}

		if val[sIdx-t.n] <= 1 {
			leafQ.Put(sIdx)
		}
	}

	// Eliminate leaves
	for leafQ.Length() > 1 {
		sIdx := leafQ.Pop()
		val[sIdx-t.n]--
		adj := t.adjacencies[sIdx-t.n]

		eqnStack.Put(sIdx)

		j := -1
		for i := range B[sIdx] {
			if B[sIdx][i] != 0 {
				j = i
			}
		}
		if j < 0 {
			panic("No neighbor found")
		}

		q0 := B[sIdx][j]
		j = adj[j]
		val[j-t.n]--
		if val[j-t.n] == 1 {
			leafQ.Put(j) // New leaf?
		}

		adj = t.adjacencies[j-t.n]
		l := -1
		for i := 0; i < 3; i++ {
			if adj[i] == sIdx {
				l = i
			}
		}
		if l < 0 {
			panic("No neighbor found")
		}

		q1 := B[j][l]
		B[j][l] = 0
		c := 1 / (1 - q0*q1)

		for i := range B[j] {
			B[j][i] = c * B[j][i]
		}

		for i := range C[j] {
			C[j][i] = c * (C[j][i] + q1*C[sIdx][i])
		}

	}

	// Solve 1-vertex tree
	i := leafQ[0]
	t.points[i] = C[i]

	// Backsolve
	for eqnStack.Length() > 0 {
		i = eqnStack.Pop()
		l := -1
		for j := range B[i] {
			if B[i][j] != 0 {
				l = j
				break
			}
		}
		if l < 0 {
			panic("No neighbor found")
		}

		q0 := B[i][l]
		adj := t.adjacencies[i-t.n]
		l = adj[l]
		for j := range t.points[i] {
			t.points[i][j] = C[i][j] + q0*t.points[l][j]
		}
	}
}

// SimpleIteration runs a single simple iteration where all points are updated
// one at a time. Each Steiner point is calculated by finding the
// Fermat-Torricelli point.
func (t *Tree) SimpleIteration() {
	n := len(t.points) - t.n
	for i := 0; i < n; i++ {
		sIdx := t.n + i
		adj := t.adjacencies[i]
		t.points[sIdx] = fermatTorricelliPoint(sIdx, adj, t)
	}
}
