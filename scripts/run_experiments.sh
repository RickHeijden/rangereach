NUM_RUNS=${1:-5}
echo "Running experiments with $NUM_RUNS runs each"

make
./scripts/run_vary-degree.sh $NUM_RUNS
./scripts/run_vary-region.sh $NUM_RUNS
./scripts/run_vary-selectivity.sh $NUM_RUNS
./scripts/run_creates.sh $NUM_RUNS
