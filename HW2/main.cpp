//
//  main.cpp
//  HW2
//
//  Created by Oleg Koptev on 14.12.2020.
//
//  10. Задача о супермаркете. В супермаркете работают два кассира,
//     покупатели заходят в супермаркет, делают покупки и становятся в очередь
//     к случайному кассиру. Пока очередь пуста, кассир спит, как только появляется
//     покупатель, кассир просыпается. Покупатель спит в очереди, пока
//     не подойдет к кассиру. Создать многопоточное приложение,
//     моделирующее рабочий день супермаркета.
//
#include <iostream>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int customersAmount;
int cashiersAmount;
const int cashiersMaxAmount = 50;

/**
 General mutex for changing customersServed variable
 */
pthread_mutex_t counterMutex;
int customersServed = 0;

/**
 Context struct for cashier thread
 */
typedef struct {
    pthread_mutex_t cameToCashierDeskMutex;
    sem_t cameToCashierDesk;
    sem_t goodsWereScanned;
    int currentCustomer;
} context_t;

context_t context_for_customers[cashiersMaxAmount];

/**
 Seed for random number generator
 */
unsigned int seed = 101;
pthread_mutex_t randomMutex;

/**
 Customer thread method
 */
void *Customer(void *param) {
    int pNum = *((int*)param);
    pthread_mutex_lock(&randomMutex);
    int deskNumber = rand() % cashiersAmount;
    pthread_mutex_unlock(&randomMutex);
    context_t *context = &context_for_customers[deskNumber];
    printf("Клиент %d встал в очередь на кассу %d\n", pNum, deskNumber + 1);
    pthread_mutex_lock(&context->cameToCashierDeskMutex);
    context->currentCustomer = pNum;
    sem_post(&context->cameToCashierDesk);
    sem_wait(&context->goodsWereScanned);
    pthread_mutex_unlock(&context->cameToCashierDeskMutex);
    return NULL;
}

/**
 Cashier thread method
 */
void *Cashier(void *param) {
    int pNum = *((int*)param);
    context_t *context = &context_for_customers[pNum - 1];
    while (1) {
        sem_wait(&context->cameToCashierDesk);
        sleep(1);
        sem_post(&context->goodsWereScanned);
        printf("Кассир %d обслужил клиента %d\n", pNum, context->currentCustomer);
        pthread_mutex_lock(&counterMutex);
        customersServed++;
        pthread_mutex_unlock(&counterMutex);
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    
    // Check if the amount of arguments is good
    if (argc != 3) {
        printf("Wrong amount of arguments. Please enter \"<cashiers amount> <customers amount>\"\n");
        return 1;
    }
    
    // Parse values from arguments list
    try {
        cashiersAmount = std::stoi(argv[1]);
        customersAmount = std::stoi(argv[2]);
    } catch (...) {
        printf("Error while parsing the numbers!\n");
    }
    
    if (cashiersAmount > cashiersMaxAmount) {
        printf("Program doesn't support cashiers amount bigger than 50\n");
    }
    
    if (cashiersAmount > customersAmount) {
        printf("Магазин иногда работает неправильно, если в нем больше кассиров чем покупателей\n");
    }
    
    printf("Магазин открыт, заходят %d покупателей\n", customersAmount);
    
    // Prepare the random number generator
    srand(seed);
    
    // Prepare all mutexes and semaphores
    pthread_mutex_init(&counterMutex, NULL);
    pthread_mutex_init(&randomMutex, NULL);
    for (int i = 0; i < cashiersAmount; i++) {
        context_t *context = &context_for_customers[i];
        pthread_mutex_init(&context->cameToCashierDeskMutex, NULL);
        sem_init(&context->cameToCashierDesk, 0, 0);
        sem_init(&context->goodsWereScanned, 0, 0);
    }
    
    // Create threads for all cashiers
    pthread_t threadCashier[customersAmount];
    int cashierNumbers[customersAmount];
    for (int i = 0; i < customersAmount; i++) {
        cashierNumbers[i] = i + 1;
        pthread_create(&threadCashier[i], NULL, Cashier, (void*)(cashierNumbers+i));
    }
    
    // Create threads for all customers
    pthread_t threadCustomer[customersAmount];
    int customerNumbers[customersAmount];
    for (int i = 0; i < customersAmount; i++) {
        customerNumbers[i] = i + 1;
        pthread_create(&threadCustomer[i], NULL, Customer, (void*)(customerNumbers+i));
    }
    
    // Main thread waits while all customers are served, then closes the shop
    while (customersServed < customersAmount) { sleep (2); }
    printf("Покупатели закончились, магазин закрыт\n");
    return 0;
}
