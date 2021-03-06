#!/bin/bash

for f in ../Instances/*json; do
	basename="$(basename $f .json).txt"

	geosteinerLen=`grep length ../GeoSteiner/$basename | tail -n 1 | tr -s ' ' | cut -d ' ' -f 9 | cut -c 1-9`
	smithNewLen=`grep -A 1 Length ../SmithNew/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`
	smithNewSortLen=`grep -A 1 Length ../SmithNewSort/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`
	simpleLen=`grep -A 1 Length ../Simple/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`
	simpleSortLen=`grep -A 1 Length ../SimpleSort/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`

	diffSmithNew=$(bc -l <<< "$smithNewLen - $geosteinerLen")
	diffSmithNewSort=$(bc -l <<< "$smithNewSortLen - $geosteinerLen")
	diffSimple=$(bc -l <<< "$simpleLen - $geosteinerLen")
	diffSimpleSort=$(bc -l <<< "$simpleSortLen - $geosteinerLen")
	diff=$(bc -l <<< "($diffSmithNew + $diffSmithNewSort + $diffSimple + $diffSimpleSort) >= 0.00001")

	ratioSmithNew=$(bc -l <<< "$diffSmithNew/$geosteinerLen*100")
	ratioSmithNewSort=$(bc -l <<< "$diffSmithNewSort/$geosteinerLen*100")
	ratioSimple=$(bc -l <<< "$diffSimple/$geosteinerLen*100")
	ratioSimpleSort=$(bc -l <<< "$diffSimpleSort/$geosteinerLen*100")

	if [ $diff -eq 1 ]; then
    	echo "INSTANCE $basename"
	    echo "LENGTH GeoSteiner      $geosteinerLen"
    	echo "LENGTH SmithNew        $smithNewLen"
    	echo "LENGTH SmithNewSort    $smithNewSortLen"
	    echo "LENGTH Simple          $simpleLen"
	    echo "LENGTH SimpleSort      $simpleSortLen"
	    echo "DIFFGEO SmithNew       $diffSmithNew / $ratioSmithNew"
	    echo "DIFFGEO SmithNewSort   $diffSmithNewSort / $ratioSmithNewSort"
    	echo "DIFFGEO Simple         $diffSimple / $ratioSimple"
    	echo "DIFFGEO SimpleSort     $diffSimpleSort / $ratioSimpleSort"
    	echo ""
	fi
done
