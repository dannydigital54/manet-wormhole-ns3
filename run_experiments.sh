echo "run,pdr,delay,throughput" > results_wormhole.csv

for i in {1..10}
do
  ./waf --run "scratch/manet-wormhole-exp --run=$i"
done

echo "Wormhole experiment completed"
