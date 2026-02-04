NUM_RUNS=${1:-5}
echo "Running experiments with ${NUM_RUNS} runs each"
for r in $(seq 1 ${NUM_RUNS})
do
  for d in foursquare gowalla weeplaces yelp
  do
      echo $d
      for m in 3dreach 3dreach_rev 2dreach 2dreach_comp 2dreach_pointer
      do
          for de in 0 1 2 3 4
          do
                  echo "./run_${m}.exec ./inputs/${d}/${d} ./queries/${d}/queries-range-degree.0.05-${de}.txt.qry > ./outputs/${d}/run_${m}_queries-range-degree.0.05-${de}_run${r}.txt"
                  ./run_${m}.exec ./inputs/${d}/${d} ./queries/${d}/queries-range-degree.0.05-${de}.txt.qry > ./outputs/${d}/run_${m}_queries-range-degree.0.05-${de}_run${r}.txt
                  echo "finished ${m} on degree ${de}"
          done
          echo
      done
      echo
  done
done
