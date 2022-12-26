// Copyright 2022 Aleksandar Simeunovic
#include<mpi.h>
#include<time.h>
#include<stdlib.h>
#include<iostream>
#include<thread>
#include<chrono>
#include<random>
#include<queue>
#include"../../../modules/task_2/simeunovic_a_slepping_barber/slepping_barber.h"
enum MessageType{REQUEST, RESPONSE, SIGNAL};
void Cutting_Hair(int client) {
    std::random_device dev;
    std::mt19937 rand_r(dev());
    int count = rand_r() % 15;
    while (count--) {
           std::cout << "Working on client with id:" << client << std::endl;
    }
     std::cout << "Ended work on client with id:" << client << std::endl;
}
struct buffer {
    int id;
    int ProcRank;
};
void Sleeping(int* mutex, bool* thread_running) {
    std::random_device dev;
    std::mt19937 rand_r(dev());
    while (*thread_running) {
        if (!(*mutex)) {
             std::cout << "Barber sleeping..." << std::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
}
void DoBarberLoop(int n, int ProcSize, int ProcRank) {
    MPI_Barrier(MPI_COMM_WORLD);
    std::random_device dev;
    std::mt19937 rand_r(dev());
    buffer message;
    MPI_Status status;
    int signal;
    bool free_barber = true;
    int mutex = 0, finished_clients = 0, thrown_clients = 0;
    bool thread_running = true;
    std::queue<buffer>ocered;
    std::thread thr(Sleeping, &mutex, &thread_running);
    int count = rand_r() %10+5;
    while (count-->0) {
        if (free_barber) {
            if (!ocered.empty()) {
                signal = 1;
                 std::cout << "Started working on client with id:" << ocered.front().id << std::endl;
                MPI_Send(&signal, 1, MPI_INT, ocered.front().ProcRank, RESPONSE, MPI_COMM_WORLD);
                ocered.pop();
                free_barber = false;
            } else {
                mutex = 0;
            }
        }
        MPI_Recv(&message, 1, MPI_2INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch (status.MPI_TAG) {
            case REQUEST: {
                if (ocered.size() < n) {
                    mutex = 1;
                        std::cout << "Customer witi id:" << message.id << " has arrived." << std::endl;
                    ocered.push(message);
                } else {
                       std::cout << "There is no place for customer with id:" << message.id << std::endl;
                    thrown_clients++;
                    signal = 0;
                    MPI_Send(&signal, 1, MPI_INT, message.ProcRank, RESPONSE, MPI_COMM_WORLD);
                }
                break;
            }
            case SIGNAL: {
                    finished_clients++;
                    free_barber = true;
            }
        }
    }
    thread_running = false;
    thr.join();
    signal = 2;
    for (int i = 1; i < ProcSize; i++) {
        MPI_Send(&signal, 1, MPI_INT, i, RESPONSE, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    Checking(ProcRank);
     // std::cout << std::endl;
     // std::cout << "The store is closed!" << std::endl;
     // std::cout << "Number of clients worked on:" << finished_clients << std::endl;
     // std::cout << "Number of clients who ended in waiting room:" << ocered.size() << std::endl;
     // std::cout << "Number of thrown clients:" << thrown_clients << std::endl;
}
void Checking(int ProcRank) {
    int last_count, count;
    bool working;
    if (ProcRank == 0) {
        last_count = 0;
        count = 0;
        working = true;
        std::thread Empty(Empty_Buffer, &working, &count);
        while (true) {
            // std::this_thread::sleep_for(std::chrono::microseconds(5));
            if (last_count == count) {
                working = false;
                break;
            } else {
                last_count = count;
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        Empty.join();
    }
    if (ProcRank != 0) MPI_Barrier(MPI_COMM_WORLD);
    if (ProcRank == 1) {
        buffer message;
        message.id = 1;
        message.ProcRank = 2;
        MPI_Send(&message, 1, MPI_2INT, 0, REQUEST, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
void Empty_Buffer(bool* working, int* count) {
    MPI_Status status;
    buffer message;
    while (*working) {
        MPI_Recv(&message, 1, MPI_2INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        *count = *count + 1;
    }
}
void ClientLoop(int ProcRank) {
    MPI_Barrier(MPI_COMM_WORLD);
    std::random_device dev;
    std::mt19937 rand_r(dev());
    buffer message;
    int signal;
    MPI_Status status;
    while (true) {
        message.id = rand_r() % 1000000;
        message.ProcRank = ProcRank;
        // std::this_thread::sleep_for(std::chrono::microseconds(5));
        MPI_Send(&message, 1, MPI_2INT, 0, REQUEST, MPI_COMM_WORLD);
        MPI_Recv(&signal, 1, MPI_INT, 0, RESPONSE, MPI_COMM_WORLD, &status);
        if (signal == 1) {
            Cutting_Hair(message.id);
            MPI_Send(&signal, 1, MPI_INT, 0, SIGNAL, MPI_COMM_WORLD);
        } else {if (signal == 2)break;}
    }
    MPI_Barrier(MPI_COMM_WORLD);
    Checking(ProcRank);
}
