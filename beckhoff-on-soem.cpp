// test sketch that shows how to use a simple I/O rack of 
#include "stdio.h"
#include "iostream"
#include <string.h>
#include "slavelist.h"
#include <inttypes.h>
#include <soem/ethercat.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <sched.h>

using namespace std;

// Digital Definitions
enum DigitalInterface {
    HIGH = true,
    LOW = false
};

// slave definitions
int predefinedSlaveList[NUMBER_OF_SLAVES] = {tree_EK1100, tree_EL1008, tree_EL2008};

pthread_t thread1, thread2;

// prog definitions
bool requestSlaveInfo = false;;
char ethName[1024];
volatile int wkc;
char IOmap[4096];

// functions
void waitForSlavesToReachSAFEOP() {
    /* wait for all slaves to reach SAFE_OP state */
    ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 3);
    if (ec_slave[0].state != EC_STATE_SAFE_OP ) {
        printf("Not all slaves reached safe operational state.\n");
        ec_readstate();

        for(int i = 1; i <= ec_slavecount; i++) {
            if(ec_slave[i].state != EC_STATE_SAFE_OP) {
                printf("Slave %d State=%2x StatusCode=%4x : %s\n",
                i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
            }
        }
    }
}

void readSlaves(bool showSlaveInfo) {
    uint16 ssigen;
    ec_readstate();
    for (int i = 1; i <= ec_slavecount; i++) {
        if (showSlaveInfo) {
            printf("\nSlave:%d\n Name:%s\n Output size: %dbits\n Input size: %dbits\n State: %d\n Delay: %d[ns]\n Has DC: %d\n",
                  i, ec_slave[i].name, ec_slave[i].Obits, ec_slave[i].Ibits,
                  ec_slave[i].state, ec_slave[i].pdelay, ec_slave[i].hasdc);
        }

        ssigen = ec_siifind(i, ECT_SII_GENERAL);
        if (ssigen) {
            ec_slave[i].CoEdetails = ec_siigetbyte(i, ssigen + 0x07);
            ec_slave[i].FoEdetails = ec_siigetbyte(i, ssigen + 0x08);
            ec_slave[i].EoEdetails = ec_siigetbyte(i, ssigen + 0x09);
            ec_slave[i].SoEdetails = ec_siigetbyte(i, ssigen + 0x0a);
            
            if((ec_siigetbyte(i, ssigen + 0x0d) & 0x02) > 0) {
                ec_slave[i].blockLRW = 1;
                ec_slave[0].blockLRW++;
            }
            
            ec_slave[i].Ebuscurrent = ec_siigetbyte(i, ssigen + 0x0e);
            ec_slave[i].Ebuscurrent += ec_siigetbyte(i, ssigen + 0x0f) << 8;
            ec_slave[0].Ebuscurrent += ec_slave[i].Ebuscurrent;
        }

        if (showSlaveInfo) {
            printf(" CoE details: %2.2x FoE details: %2.2x EoE details: %2.2x SoE details: %2.2x\n",
                    ec_slave[i].CoEdetails, ec_slave[i].FoEdetails, ec_slave[i].EoEdetails, ec_slave[i].SoEdetails);
            printf(" Ebus current: %d[mA]\n only LRD/LWR:%d\n",
                    ec_slave[i].Ebuscurrent, ec_slave[i].blockLRW);
        }
    }
}

void showSlaveTree(bool successInit) {
    switch (successInit) {
    case true:
        cout << ec_slave[tree_EK1100].name << " (" << tree_EK1100 << ") " << endl;
        for (int i = tree_EK1100 + 1; i < LastSlave; i++) {
            cout << "|--(" << i << ")--> " << ec_slave[i].name << ": " << "Output [bits]: " << ec_slave[i].Obits << " Input [bits]: " << ec_slave[i].Ibits << endl;
        }
        break;
    case false:
        break;
    }
}

bool initSlaves(char* portName, bool requestSlaveInfo) { //initiate slaves
    int expectedWKC;
    if ( ec_init(portName) ){
        printf("ec_init on %s succeeded.\n", portName);

        if ( ec_config(FALSE, &IOmap) > 0 ) {
            ec_configdc();
            while(EcatError) printf("%s", ec_elist2string());
            printf("%d slaves found and configured.\n",ec_slavecount);

            expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
            printf("Calculated workcounter %d\n", expectedWKC);
            printf("\n...building EtherCAT slave I/O tree...");
            waitForSlavesToReachSAFEOP();
            readSlaves(requestSlaveInfo);

            printf(" done\n\n");

            showSlaveTree(true);
            return true;
        }
    }   
}

bool requestAllSlavesToOPSTATE() {
    bool slavesInOpState = false;
    printf("\n...setting all slaves to OP STATE...");
    ec_slave[0].state = 0x08;

    /* send one valid process data to make outputs in slaves happy*/
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    
    /* request OP state for all slaves */
    ec_writestate(0);
    
    /* wait for all slaves to reach OP state */
    while ( !slavesInOpState ) {
        for (int i = tree_EK1100 - 1; i < LastSlave; i++) {
            ec_slave[i].state = 0x08;
        }

        ec_send_processdata();
        ec_receive_processdata(EC_TIMEOUTRET);
        ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);

        slavesInOpState = true;
        for (int i = tree_EK1100 - 1; i < LastSlave; i++) {
            slavesInOpState = slavesInOpState && (ec_slave[i].state == 0x08);
            cout << ".";
        }
    }

    printf(" done \n\n");
    return true;
}

uint8_t* uint8ToBinaryArray(uint8_t input) {
    static uint8_t result[8]; //smallest size array of 8 bit
    static int intInput = unsigned(input); // upscale type casting not too efficient but should work
    for (int i = 7; i >= 0 || intInput != 0; i--) {
        result[i] = ( intInput % 2 == 0) ? 1 : 0; 
        intInput /= 2;
    }

    return result;
}

void digitalWrite(uint32_t slaveNumber, uint8 moduleIndex, bool value) {    
    if (moduleIndex <= 8) {
        switch (value) {
        case HIGH:
            *ec_slave[slaveNumber].outputs |= ( 1 << (moduleIndex - 1) ); // 00000001 << how many times move 1 to left register
            break;
        case LOW:
            *ec_slave[slaveNumber].outputs &= ( 1 << (moduleIndex - 1) );
            break;
        }
    }
}

bool digitalRead(uint32_t slaveNumber, int8_t moduleIndex) {
    uint8_t* value;
    value = uint8ToBinaryArray(*ec_slave[slaveNumber].inputs);

    if ( moduleIndex > 9 ) { return 0; };

    return ( unsigned(value[7 - (moduleIndex - 1)]) == 1 ) ? HIGH : LOW;
}

OSAL_THREAD_FUNC_RT ecat_write_io() {
    bool turnonall = true;
    int i = 1;
    ec_send_processdata();
    while (true) {
        turnonall = ( turnonall && (i < 9) ) || ( !turnonall && (i == 1) );
        switch (turnonall) {
        case true:  
            digitalWrite(tree_EL2008, i, HIGH);
            if (i < 8) { i++; };
            break;            
        case false:
            digitalWrite(tree_EL2008, i, LOW);
            if (i > 1) { i--; };
            break;
        }
        ec_send_processdata();
    }
}

OSAL_THREAD_FUNC_RT ecat_read_io() {
    while (true) {
        for (int i = 1; i <= 8; i++) {
            ec_send_processdata();
            printf("Value at bit %d is %d \n", i, digitalRead(tree_EL1008, i));
        }
    }
}

/*
    build with 
    g++ -lpthread beckhoff-on-soem.cpp -o demo-arm64 -g `pkg-config --libs --cflags soem`

*/

int ecat_read_rt_result, ecat_write_rt_result;

int main(int argc, char* argv[]) { // argv[0] is the 
    // 1. initialize slave devices on the specified ethernet port
    int ctime = 1000; // 1000us = 1ms
    switch (argc) {
    case 1:
        cout << "no eth port provided" << endl;
        return 0;
    default:
        strcpy(ethName, argv[1]);
        if (argv[2] == "slaveinfo") {
            requestSlaveInfo = true;
        }
    }

    if (!initSlaves(ethName, requestSlaveInfo)) { return 0; }; // here all slaves must be at 0x04 state -> SAFEOP STATE

    // 2. set all devies to OP STATE prior running them
    if ( !requestAllSlavesToOPSTATE() ) { return 0; };  // all slaves must be in 0x08 state -> OP STATE

    // 3. run simple program
    ecat_read_rt_result = osal_thread_create_rt(&thread1, 1024, ecat_read_io, (void*)&ctime);
    if ( !ecat_read_rt_result ) {
        return 0;
    }

    ecat_write_rt_result = osal_thread_create_rt(&thread2, 1024, ecat_write_io, (void*)&ctime);
    if ( !ecat_write_rt_result ) {
        return 0;
    }

    return 0;
}