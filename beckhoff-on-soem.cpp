// test sketch that shows how to use a simple I/O rack of 
#include "stdio.h"
#include "iostream"
#include <string.h>
#include "slavelist.h"
#include <inttypes.h>
#include <soem/ethercat.h>

using namespace std;

// slave definitions
int predefinedSlaveList[NUMBER_OF_SLAVES] = {EK1100, EL1008, EL2008};

// prog definitions
char ethName[1024];

// functions
void waitForSlavesToReachSAFEOP() {
    /* wait for all slaves to reach SAFE_OP state */
    ec_statecheck(0, EC_STATE_SAFE_OP,  EC_TIMEOUTSTATE * 3);
    if (ec_slave[0].state != EC_STATE_SAFE_OP ) {
        printf("Not all slaves reached safe operational state.\n");
        ec_readstate();
    for(int i = 1; i <= ec_slavecount ; i++) {
        if(ec_slave[i].state != EC_STATE_SAFE_OP) {
                printf("Slave %d State=%2x StatusCode=%4x : %s\n",
                i, ec_slave[i].state, ec_slave[i].ALstatuscode, ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
            }
        }
    }
}

void readSlaves() {
    uint16 ssigen;
    for (int i = 0; i < ec_slavecount; i++) {
        printf("\nSlave:%d\n Name:%s\n Output size: %dbits\n Input size: %dbits\n State: %d\n Delay: %d[ns]\n Has DC: %d\n",
                  i, ec_slave[i].name, ec_slave[i].Obits, ec_slave[i].Ibits,
                  ec_slave[i].state, ec_slave[i].pdelay, ec_slave[i].hasdc);

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

        printf(" CoE details: %2.2x FoE details: %2.2x EoE details: %2.2x SoE details: %2.2x\n",
                    ec_slave[i].CoEdetails, ec_slave[i].FoEdetails, ec_slave[i].EoEdetails, ec_slave[i].SoEdetails);
        printf(" Ebus current: %d[mA]\n only LRD/LWR:%d\n",
                    ec_slave[i].Ebuscurrent, ec_slave[i].blockLRW);
    }
}

bool initSlaves(char* portName) { //initiate slaves
    int expectedWKC;
    if ( !(ec_init(ethName) > 0) ) {
        return false;
    } else if (ec_init(ethName) > 0) {
        printf("ec_init on %s succeeded.\n", ethName);

        ec_configdc();
        while(EcatError) printf("%s", ec_elist2string());
        printf("%d slaves found and configured.\n",ec_slavecount);

        expectedWKC = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
        printf("Calculated workcounter %d\n", expectedWKC);
        waitForSlavesToReachSAFEOP();
        readSlaves();

        return true;
    }   
}

void setOutput(uint32 slaveAddress, int pin, bool value) { //set output at bit X

}

void readInput() { // read input at bit X

}

int main(int argc, char* argv[]) { // argv[0] is the 
    // 1. initialize slave devices on the specified ethernet port
    switch (argc) {
    case 0:
        return 0;
    case 1:
        strcpy(ethName, argv[1]);
    }

    if (!initSlaves(ethName)) { return 0; };

    // 2. execute simple program below


    return 0;
}