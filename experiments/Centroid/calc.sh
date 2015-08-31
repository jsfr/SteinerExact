echo "n=10"
echo -n "iterations: "
cat cube_n10* | grep iterations | cut -d ':' -f 2 | awk '{a+=$1} END{print a/NR}'
echo -n "trees: "
cat cube_n10* | grep trees | cut -d ':' -f 2 | awk '{a+=$1} END{print a/NR}'
echo ""

echo "n=11"
echo -n "iterations: "
cat cube_n11* | grep iterations | cut -d ':' -f 2 | awk '{a+=$1} END{print a/NR}'
echo -n "trees: "
cat cube_n11* | grep trees | cut -d ':' -f 2 | awk '{a+=$1} END{print a/NR}'
echo ""

echo "n=12"
echo -n "iterations: "
cat cube_n12* | grep iterations | cut -d ':' -f 2 | awk '{a+=$1} END{print a/NR}'
echo -n "trees: "
cat cube_n12* | grep trees | cut -d ':' -f 2 | awk '{a+=$1} END{print a/NR}'
