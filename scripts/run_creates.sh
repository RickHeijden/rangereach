NUM_RUNS=${1:-5}
echo "Running experiments with ${NUM_RUNS} runs each"
for r in $(seq 1 ${NUM_RUNS})
do
  for d in foursquare gowalla weeplaces yelp
  do
    echo "./scripts/create_dag.sh ./inputs/${d}/${d} > ./outputs/${d}/create_scripts_run_${r}.txt"
    ./scripts/create_dag.sh ./inputs/${d}/${d} > ./outputs/${d}/create_scripts_run_${r}.txt
    echo "./create_int.exec ./inputs/${d}/${d} >> ./outputs/${d}/create_scripts_run_${r}.txt"
    ./create_int.exec ./inputs/${d}/${d} >> ./outputs/${d}/create_scripts_run_${r}.txt
    echo "./create_int.exec ./inputs/${d}/${d} reverse >> ./outputs/${d}/create_scripts_run_${r}.txt"
    ./create_int.exec ./inputs/${d}/${d} reverse >> ./outputs/${d}/create_scripts_run_${r}.txt
    echo "finished run ${r} - dataset ${d}"
  done
done
