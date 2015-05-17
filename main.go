package main

func main() {
	cfg := InitConfig()
	tree := cfg.ParseTree()

	sprouter := func(t Tree, idx int, done chan bool) {
		t.Sprout(idx)
		t.Print()
		done <- true
	}

	chans := make([]chan bool, len(tree.Edges))

	for i := 0; i < len(tree.Edges); i++ {
		ch := make(chan bool)
		chans[i] = ch
		go sprouter(tree, i, chans[i])
	}

	for _, ch := range chans {
		_ = <-ch
	}
}
