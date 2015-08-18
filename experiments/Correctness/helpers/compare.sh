#!/bin/bash

for f in ../Instances/*json; do
	basename="$(basename $f .json).txt"

	geosteinerLen=`grep length ../GeoSteiner/$basename | tail -n 1 | tr -s ' ' | cut -d ' ' -f 9 | cut -c 1-9`
	smithLen=`grep -A 1 Length ../Smith/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`
	simpleLen=`grep -A 1 Length ../Simple/$basename | tail -n 1 | sed 's/\t/ /g' | tr -s ' ' | cut -d ' ' -f 2 | cut -c 1-9`

	diffGeosteiner=$(qalc -t $geosteinerLen - $geosteinerLen)
	diffSmith=$(qalc -t $smithLen - $geosteinerLen)
	diffSimple=$(qalc -t $simpleLen - $geosteinerLen)

	echo "INSTANCE $basename"
	echo "LENGTH GeoSteiner   $geosteinerLen"
	echo "LENGTH Smith        $smithLen"
	echo "LENGTH Simple       $simpleLen"

	echo "DIFFGEO GeoSteiner  $diffGeosteiner"
	echo "DIFFGEO Smith       $diffSmith"
	echo "DIFFGEO Simple       $diffSimple"
done
