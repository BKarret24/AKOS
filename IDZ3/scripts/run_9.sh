#!/bin/bash

echo "Запуск shm_mgr..."
./bin/shm_mgr &
sleep 1

echo "Запуск наблюдателя..."
./bin/obs &
sleep 1

echo "Запуск продавца A..."
./bin/seller A &
sleep 1

echo "Запуск продавца B..."
./bin/seller B &
sleep 1

echo "Запуск покупателей..."
./bin/buyer 0 &
./bin/buyer 1 &
./bin/buyer 2 &

echo "Все процессы запущены."
wait
