NUM_RUNS=${1:-5}
echo "Running experiments with $NUM_RUNS runs each"
for r in $(seq 1 $NUM_RUNS)
do
  for d in foursquare gowalla weeplaces yelp
  do
      echo $d
      for m in 3dreach 3dreach_rev 2dreach 2dreach_comp 2dreach_pointer
      do
          for s in 0.00001 0.0001 0.001 0.01
          do
                  echo "./run_${m}.exec ./inputs/${d}/${d} ./queries/${d}/queries-selectivity.${s}.txt.qry > ./outputs/${d}/run_${m}_queries-selectivity.${s}_run${r}.txt"
                  ./run_${m}.exec ./inputs/${d}/${d} ./queries/${d}/queries-selectivity.${s}.txt.qry > ./outputs/${d}/run_${m}_queries-selectivity.${s}_run${r}.txt
          done
          echo
      done
      echo
  done
done
