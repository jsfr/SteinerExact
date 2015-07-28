# About #
This project contains a implementation of the algorithm for finding Steiner
Minimal Trees in higher dimensions, as described by Smith in
[this article](http://link.springer.com/article/10.1007%2FBF01758756) from
Algorithmica, 1992.

The implementation tries to do some things smarter than the original
implementation by Smith, e.g. by not rebuilding the topology every time a point
is added or removed.

Furthermore the implementation also implements another iteration, apart from the
one described by Smith. This iteration uses a analytical method to place Steiner
points immediatly at the correct place.

The implementation also has the ability to sort points before optimizing. The
sorting is an approximation of the longest path through a fully connected graph.

# Dependencies #
- golang 1.4 or newer

# Building #
Simply run `make` and everything should be done.

# Usage #
Trees should be defined in separate JSON-files containing a key `"points"`
having a list of all the points of the tree.

After building the program it can be used to optimize a tree defined in some
file e.g. `example.json` by calling `./steinertree example.json`

The program currently has the following flags which can be used:
- `-1` : Change the index of edges and points in the output to start at 1
  instead of 0. This is useful for comparing with the old implementation which
  is 1-indexed.
- `-sort` : Sort the terminals before optimizing.
- `-iteration=[itername]` : Select which iteration to use for
  optimization. Possiblities are: `simple` and `smith`. Default: `smith`
