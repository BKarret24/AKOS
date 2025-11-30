#!/bin/bash

echo "Запуск shm_mgr..."
./bin/shm_mgr &
sleep 1

echo "Запуск диспетчера..."
./bin/disp10 &
sleep 1

echo "Запуск наблюдателей..."
./bin/obs10 0 &
./bin/obs10 1 &
sleep 1

echo "Запуск продавцов..."
./bin/seller A &
./bin/seller B &
sleep 1

echo "Запуск покупателей..."
./bin/buyer 0 &
./bin/buyer 1 &
./bin/buyer 2 &

echo "Все процессы запущены."
wait

