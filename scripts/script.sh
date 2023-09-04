#!/bin/bash
echo $BASH_SOURCE

folder="scripts/"
clscripts=("inputA" "inputB" "inputC")
clpids=()
files=()

echo "Server starting"
./server < "$folder"inputS > inputS.stdout &
serpid=$!
files+=("inputS.stdout")

echo "Clients starting"
for i in "${clscripts[@]}"; do
    ./client < "$folder$i" > "$i.stdout" &
    clpids+=("$!")
    files+=("$i.stdout")
done
for i in "${clpids[@]}"; do
    wait "$i"
done
echo "Clients finished"

wait $serpid
echo "Server finished"

echo "Check these files for output:"
for i in "${files[@]}"; do
    echo "- $i"
done