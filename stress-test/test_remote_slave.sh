#!/bin/bash
ssh $1 bash << EOF
	mkdir -p /tmp/roberto-stress-test/
	rm -rf /tmp/roberto-stress-test/*
EOF
scp -r $(pwd) $1:/tmp/roberto-stress-test
ssh -t $1 bash << EOF
	sudo ip link set $2 promisc on
	cd /tmp/roberto-stress-test/stress-test/
	poetry env use python3.10
	poetry install
	poetry run test_slave -i $2 --port $3
EOF
