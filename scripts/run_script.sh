#!/bin/bash
./build/OrderBook_Server &
./build/OrderBook_Client && ./build/test/OrderBook_Test
