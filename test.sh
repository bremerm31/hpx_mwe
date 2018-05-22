#!/bin/bash
N_runs=100
fail_count=0

for ((i=0; i <${N_runs}; i++)); do
    mpirun -np 2 build/DG_HYPER_SWE_HPX --hpx:threads=1
    if [ $(echo $? -ne 0) ]; then
	echo "Test failed"
	((fail_count++))
    fi
done
echo "Summary:"
echo "  ${fail_count} failed out of ${N_runs} runs"