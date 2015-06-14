package smt

// SmithsIteration runs a single of iteration as proposed by Smith.
// TODO: This implementation is more or less shamelessly copied from
// Smiths implementation. If there is time, it would be nice to try implementing
// the gaussian elimination better, and possibly using goroutines. E.g. we could
// have a routine for each dimension as these are actually separate.
func (t *Tree) SmithsIteration() {
	// Set up equations t.points[t.n+i][] = sum_{j}( B[i][j] * t.points[j][] ) + C[i]

	B := make(map[int][]float64)
	C := make(map[int][]float64)
	val := make(map[int]int)
	leafQ := stack{}
	eqnStack := stack{}

	// Prepare equations
	for sIdx, eIdx := range t.adjacencies {
		epsilon := 0.0001 * t.Error() / float64(t.n)
		pIdx := AdjacentPoints(sIdx, t)
		q := [3]float64{
			1 / (t.edges[eIdx[0]].Length() + epsilon),
			1 / (t.edges[eIdx[1]].Length() + epsilon),
			1 / (t.edges[eIdx[2]].Length() + epsilon),
		}
		sum := q[0] + q[1] + q[2]
		q[0] = q[0] / sum
		q[1] = q[1] / sum
		q[2] = q[2] / sum

		B[sIdx] = make([]float64, 3)
		C[sIdx] = make([]float64, t.dim)
		for a := 0; a < 3; a++ {
			b := pIdx[a]
			c := q[a]
			i := sIdx
			if b >= t.n {
				val[i]++
				B[i][a] = c
			} else {
				for m := range C[i] {
					C[i][m] = C[i][m] + t.points[b][m]*c
				}
			}
		}

		if val[sIdx] <= 1 {
			leafQ.Put(sIdx)
		}
	}

	// Eliminate leaves
	for leafQ.Length() > 1 {
		sIdx := leafQ.Pop()
		val[sIdx]--
		pIdx := AdjacentPoints(sIdx, t)

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
		j = pIdx[j]
		val[j]--
		if val[j] == 1 {
			leafQ.Put(j) // New leaf?
		}

		pIdx2 := AdjacentPoints(j, t)
		l := -1
		for i := range pIdx2 {
			if pIdx2[i] == sIdx {
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
	for _, j := range t.adjacencies[i] {
		t.edges[j].UnsetLength()
	}

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
		pIdx3 := AdjacentPoints(i, t)
		l = pIdx3[l]
		for j := range t.points[i] {
			t.points[i][j] = C[i][j] + q0*t.points[l][j]
			for _, j := range t.adjacencies[i] {
				t.edges[j].UnsetLength()
			}
		}
	}
}

// SimpleIteration runs a single simple iteration where all points a updated one
// at a time. Each Steiner point is calculated by finding the Fermat-Torricelli
// point.
func (t *Tree) SimpleIteration() {
	for sIdx, eIdx := range t.adjacencies {
		pIdx := AdjacentPoints(sIdx, t)
		t.points[sIdx] = FermatTorricelliPoint(pIdx, t)
		for _, i := range eIdx {
			t.edges[i].UnsetLength()
		}
	}
}
