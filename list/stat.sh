echo "average teleport distance"
cat graphs/listHTZ-avgTelDist-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listLTZ-avgTelDist-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listFTZ-avgTelDist-m20-i5000.out | sed s/"\t"/" \t"/g
echo "Commit rates"
cat graphs/listHTZ-rate-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listLTZ-rate-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listFTZ-rate-m20-i5000.out | sed s/"\t"/" \t"/g

echo "Abort reasons for hand over hand: capacity, conflict, explicit, unknown"
cat graphs/listHTZ-capacity-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listHTZ-conflict-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listHTZ-explicit-m20-i5000.out | sed s/"\t"/" \t"/g
cat graphs/listHTZ-unknown-m20-i5000.out | sed s/"\t"/" \t"/g


