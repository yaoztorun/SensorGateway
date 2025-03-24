make all
port=5678
clients=6
echo -e "starting gateway "
./sensor_gateway $port $clients &
sleep 3
echo -e 'starting 3 sensor nodes'
./sensor_node 15 1 127.0.0.1 $port &
sleep 2
./sensor_node 49 3 127.0.0.1 $port &
sleep 22
./sensor_node 21 2 127.0.0.1 $port &
sleep 11
./sensor_node 39 1 127.0.0.1 $port &
sleep 2
./sensor_node 129 3 127.0.0.1 $port &
sleep 22
./sensor_node 132 2 127.0.0.1 $port &
sleep 15
killall sensor_node
sleep 20
killall sensor_gateway


# chmod +x test1.sh
# ./test3.sh
