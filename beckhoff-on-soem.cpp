// test sketch that shows how to use a simple I/O rack of 
#include "stdio.h"
#include "iostream"
#include <string.h>
#include "slavelist.h"
#include "soem/ethercat.h"

using namespace std;

// slave definitions
int predefinedSlaveList[NUMBER_OF_SLAVES] = {EK1100, EL1008, EL2008};

// prog definitions
char* ethName = "";

// functions
bool initSlaves(char* portName) { //initiate slaves
    if ( !(ec_init(ethName) > 0) ) {
        return false;
    } else if (ec_init(ethName) > 0) {
        printf("ec_init on %s succeeded.\n", ethName);

        ec_configdc();

    }   
}

void setOutput(uint32 slaveAddress, std::byte pin, bool value) { //set output at bit X

}

void readInput() { // read input at bit X

}

int main(int argc, char *argv[]) { // argv[0] is the 
    // 1. initialize slave devices on the specified ethernet port
    switch (argc) {
    case 0:
        return 0;
    case 1:
        ethName = argv[1];
    }

    if (!initSlaves(ethName)) { return 0; };

    // 2. execute simple program below


    return 0;
}