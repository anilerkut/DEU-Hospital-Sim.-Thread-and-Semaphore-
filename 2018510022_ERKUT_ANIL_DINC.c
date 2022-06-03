#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <stdbool.h>

//Room numbers
#define REGISTRATION_SIZE 10
#define RESTROOM_SIZE 10
#define CAFE_NUMBER 10
#define GP_NUMBER 10
#define PHARMACY_NUMBER 10
#define BLOOD_LAB_NUMBER 10
#define OR_NUMBER 10
//doctor and nurse number
#define SURGEON_NUMBER 30
#define NURSE_NUMBER 30
//doctor and nurse limit
#define SURGEON_LIMIT 5
#define NURSE_LIMIT 5
//patient number
#define PATIENT_NUMBER 1000
int HOSPITAL_WALLET = 0;
//Time limits
#define ARRIVAL_TIME 100
#define WAIT_TIME 100
#define REGISTRATION_TIME 100
#define GP_TIME 200
#define PHARMACY_TIME 100
#define BLOOD_LAB_TIME 200
#define SURGERY_TIME 500
#define CAFE_TIME 100
#define RESTROOM_TIME 100
//Cost Limits
//SURGERY_OR_COST + (number of surgeons * SURGERY_SURGEON_COST) +(number of nurses * SURGERY_NURSE_COST)
#define REGISTRATION_COST 100
#define PHARMACY_COST 200
#define BLOOD_LAB_COST 200
#define SURGERY_OR_COST 200
#define SURGERY_SURGEON_COST 100
#define SURGERY_NURSE_COST 50
#define CAFE_COST 200
//Patient properties
#define HUNGER_INCREASE_RATE 10
#define RESTROOM_INCREASE_RATE 10
//initialize patient
#define HUNGER_METER 100
#define RESTROOM_METER 100
#define DISEASE 3   // 0=> needs medicine 1=> needs blood lab 2=> needs surgery 3=> healthy

sem_t semaphore_regist;
sem_t semaphore_gp;
sem_t semaphore_pharmacy;
sem_t semaphore_bloodLab;
sem_t semaphore_surgery;
sem_t semaphore_restroom;
sem_t semaphore_cafe;

typedef struct thread  //patient struct => thread
{
    pthread_t thread_id;
    int patientnumber;
    int hungermeter;
    int restroommeter;
    int disease; 
}Patient;

int randomnumber(int limit);
void timer(int limit);
void updaterHungerRestroom(Patient *patient);
void controllerHungerRestroom(Patient *patient);
void* registiration(void* args);
void general_practitioner(Patient *args);
void pharmacy(Patient *args);
void bloodLab(Patient *args);

//struct thread reference => https://stackoverflow.com/questions/20480529/thread-struct-as-function-parameter-c
//semaphore reference => https://www.youtube.com/watch?v=YSn8_XdGH7c

int randomnumber(int limit)
{
    int random;
    random=rand()%limit;
    return random;
}

void timer(int limit) //it creates random time according to time limit
{
    int random_time=randomnumber(limit);
    usleep(1000*(random_time+1));
}

void updaterHungerRestroom(Patient *patient) //update hunger and restroom needs
{
    int hungerrate=randomnumber(HUNGER_INCREASE_RATE);
    int restroomrate=randomnumber(RESTROOM_INCREASE_RATE);
    patient->hungermeter+=hungerrate;
    patient->restroommeter+=restroomrate;
}

void controllerHungerRestroom(Patient *patient)
{
    if(patient->hungermeter>=100)
    {
        while(true)
        {
            if(sem_trywait(&semaphore_cafe)!=0) //if there are no capacity, thread will not be able to continue operations from below.
            {    
                printf(" Patient %d Waiting for Cafe...\n",patient->patientnumber);
                timer(WAIT_TIME); //cafe lineup
            }
            else
            {
                printf(" Patient %d Entering Cafe...\n",patient->patientnumber);
                timer(CAFE_TIME); //patient wait for the cafe operations
                patient->hungermeter=0;
                sem_post(&semaphore_cafe); //cafe capacity increased by one
                printf(" Patient %d Logged out from Cafe...\n",patient->patientnumber);
                break;
            }
        }
    }
    if(patient->restroommeter>=100)
    {
        while(true)
        {
            if(sem_trywait(&semaphore_restroom)!=0) //if there are no capacity, thread will not be able to continue operations from below.
            {    
                printf(" Patient %d Waiting for Restroom...\n",patient->patientnumber);
                timer(WAIT_TIME); //restroom lineup
            }
            else
            {
                printf(" Patient %d Entering Restroom...\n",patient->patientnumber);
                timer(RESTROOM_TIME); //patient wait for the restroom operations
                patient->restroommeter=0;
                sem_post(&semaphore_restroom); //restroom capacity increased by one
                printf(" Patient %d Logged out from Restroom...\n",patient->patientnumber);
                break;
            }
        }
    }
}

void* registiration(void* args) //firstly patients go to the registiration office 
{
    Patient *patient=&(*(Patient*)args);
    int a=0;
    while(true)
    {
        if(sem_trywait(&semaphore_regist)!=0) //if there are no capacity, thread will not be able to continue operations from below.
        {    
            printf("Patient %d Waiting for registiration room...\n",(*(Patient*)args).patientnumber);
            timer(WAIT_TIME);
            updaterHungerRestroom(patient); //updating hunger and restroom rates
            controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
        }
        else
        {
            printf("Patient %d Entering registiration room...\n",(*(Patient*)args).patientnumber);
            timer(REGISTRATION_TIME);
            sem_post(&semaphore_regist); //registiration capacity increased by one
            printf("Patient %d Logged out from registiration room...\n",(*(Patient*)args).patientnumber);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            break;
        }
    }
    controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
    HOSPITAL_WALLET+=REGISTRATION_COST;
    general_practitioner(patient);   //after registiration patients go to examination.
}


void general_practitioner(Patient *args)
{
    Patient *patient=&(*(Patient*)args);
    while(true)
    {
        if(sem_trywait(&semaphore_gp)!=0) //if there are no capacity, thread will not be able to continue operations from below.
        {   
            printf("Patient %d Waiting for GP room...\n",patient->patientnumber);
            timer(WAIT_TIME);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
        }
        else
        {
            printf("Patient %d Entering GP room...\n",patient->patientnumber);
            timer(GP_TIME);
            sem_post(&semaphore_gp); //gp capacity increased by one
            printf("Patient %d Logged out from GP room...\n",patient->patientnumber);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            break;
        }
    }
    controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
   
    //after examination patients go to one of three offices.
    if(patient->disease==0)
    {
        pharmacy(patient);   //after gp if necessary, patients go to pharmacy for buy drug.
    }
    else if(patient->disease==1)
    {     
        bloodLab(patient);   //after gp if necessary, patients go to blood lab.
    }
    else if(patient->disease==2)
    {
        printf("Patient %d Needs surgery\n",patient->patientnumber);
    }
    else if(patient->disease==3)
    {
        printf("Good Bay Patient %d \n",patient->patientnumber); //after pharmacy or surgery, if patient is healthy then thread exit
    }
}

void pharmacy(Patient *args)
{
    Patient *patient=&(*(Patient*)args);
    while(true)
    {
        if(sem_trywait(&semaphore_pharmacy)!=0) //if there are no capacity, thread will not be able to continue operations from below.
        {   
            printf("Patient %d Waiting for pharmacy...\n",patient->patientnumber);
            timer(WAIT_TIME);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
        }
        else
        {
            printf("Patient %d Entering pharmacy...\n",patient->patientnumber);
            timer(PHARMACY_TIME);
            sem_post(&semaphore_pharmacy); //pharmacy capacity increased by one
            printf("Patient %d Logged out from pharmacy...\n",patient->patientnumber);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            break;
        }
    }
    controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
    int pharmacy_cost=randomnumber(PHARMACY_COST); //add randomly selected medicine cost to Hospital wallet
    HOSPITAL_WALLET+=pharmacy_cost;  
    printf("Good Bay Patient %d \n",patient->patientnumber); //after pharmacy patient will be out of the hospital
}

void bloodLab(Patient *args)
{
    Patient *patient=&(*(Patient*)args);
    while(true)
    {
        if(sem_wait(&semaphore_bloodLab)) //if there are no capacity, thread will not be able to continue operations from below.
        {   
            printf("Patient %d Waiting for Blood Lab...\n",patient->patientnumber);
            timer(WAIT_TIME);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
        }
        else
        {
            printf("Patient %d Entering Blood Lab...\n",patient->patientnumber);
            timer(BLOOD_LAB_TIME);
            sem_post(&semaphore_bloodLab); //bloodlab capacity increased by one
            printf("Patient %d Logged out from Blood Lab...\n",patient->patientnumber);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            break;
        }
    }
    controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
    int needMedicine=rand()%2;  //patient needs for medicine or  he/she is healthy.
    patient->disease=needMedicine;
    if(patient->disease==1)
    {
        patient->disease=3;   //Patients is healthy no need medicine , 3 meaning healthy
    }
    if(patient->disease==0)
    {  
        general_practitioner(patient);   //Patient needs medicine
    }
    else if(patient->disease==3)
    {
        general_practitioner(patient);  //Patient is healthy
    }
    HOSPITAL_WALLET+=BLOOD_LAB_COST; //add blood lab cost to wallet
}
/*
void surgery(Patient patient)
{
    int nurse=randomnumber(NURSE_NUMBER);
    int doctor=randomnumber(SURGEON_NUMBER);
    while(true)
    {
        if(sem_wait(&semaphore_surgery)) //if there are no capacity, thread will not be able to continue operations from below.
        {   
            printf("Patient %d Waiting for Surgery...\n",patient->patientnumber);
            timer(WAIT_TIME);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
        }
        else
        {
            printf("Patient %d Entering Surgery...\n",patient->patientnumber);
            timer(SURGERY_TIME);
            sem_post(&semaphore_surgery); //bloodlab capacity increased by one
            printf("Patient %d Logged out from SURGERY...\n",patient->patientnumber);
            updaterHungerRestroom(patient); //updating hunger and restroom rate
            break;
        }
    }
    controllerHungerRestroom(patient); //controlling patients hunger and restroom rates
}
*/
int main(int argc, char *argv[])
{
    srand(time(NULL));
    int error;
    Patient patients[PATIENT_NUMBER];
    //semaphores are initialized
    sem_init(&semaphore_pharmacy,0,PHARMACY_NUMBER);
    sem_init(&semaphore_regist,0,REGISTRATION_SIZE);
    sem_init(&semaphore_bloodLab,0,BLOOD_LAB_NUMBER);
    sem_init(&semaphore_gp,0,GP_NUMBER);
    sem_init(&semaphore_cafe,0,CAFE_NUMBER);
    sem_init(&semaphore_surgery,0,OR_NUMBER);
    sem_init(&semaphore_restroom,0,RESTROOM_SIZE);
    for (int i = 0; i <PATIENT_NUMBER; i++)
    {
        //specify patient's features
        int random_hunger=randomnumber(HUNGER_METER);
        int random_restroom=randomnumber(RESTROOM_METER); 
        int random_disease=randomnumber(DISEASE);
        //add patient's features
        patients[i].hungermeter=random_hunger;  
        patients[i].restroommeter=random_restroom;
        patients[i].disease=random_disease;
        patients[i].patientnumber=i;
        printf("Patient %d has arrived to hospital\n",i);
        error=pthread_create(&(patients[i].thread_id),NULL,&registiration,(void*)&patients[i]); //thread created
        timer(ARRIVAL_TIME);
        if(error!=0)
        {
            printf("Thread creation error\n");
        }
    }
    for (int i = 0; i < PATIENT_NUMBER; i++)
    {
       error=pthread_join(patients[i].thread_id,NULL)!=0;
       if(error!=0)
       {
           printf("Failed to join thread");
       }
    }
    //semaphores are destroyed.
    sem_destroy(&semaphore_regist);
    sem_destroy(&semaphore_pharmacy);
    sem_destroy(&semaphore_bloodLab);
    sem_destroy(&semaphore_gp);
    sem_destroy(&semaphore_cafe);
    sem_destroy(&semaphore_restroom);
    sem_destroy(&semaphore_surgery);
    printf("Hospital Wallet: %d\n",HOSPITAL_WALLET);
    return 0;
}









