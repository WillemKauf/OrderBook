#!/bin/bash
sudo docker build -t orderbook_image .
sudo docker run --network host -p 8888:8888/udp orderbook_image
