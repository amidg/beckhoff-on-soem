// test sketch that shows how to use a simple I/O rack of 
#include "stdio.h"
#include "iostream"
#include <string.h>
#include "slavelist.h"
#include <inttypes.h>
#include <soem/ethercat.h>

using namespace std;

// Digital Definitions
enum DigitalInterface {
    HIGH = true,
    LOW = false
};

// slave definitions
int predefinedSlaveList[NUMBER_OF_SLAVES] = {EK1100, EL1008, EL2008};

EL1008_slave EL1008_device;

// prog definitions
bool requestSlaveInfo = false;;
char ethName[1024];
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
        cout << ec_slave[EK1100].name << " (" << EK1100 << ") " << endl;
        for (int i = EK1100 + 1; i < LastSlave; i++) {
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
    printf("\n...setting all slaves to OP STATE...");
    ec_slave[0].state = EC_STATE_OPERATIONAL;

    /* send one valid process data to make outputs in slaves happy*/
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    
    /* request OP state for all slaves */
    ec_writestate(0);
    
    /* wait for all slaves to reach OP state */
    ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);

    printf(" done \n\n");
    return true;
}

void setDigitalOutput(uint32 slaveNumber, uint8 moduleIndex, bool value) {
    uint8 startbit = ec_slave[slaveNumber].Ostartbit;

    /* Move pointer to correct module index*/
    //data_ptr += moduleIndex * 2;

    for (int i = 0; i < 8; i++) {
        cout << startbit << endl;
    }

    // Read value byte by byte since all targets can't handle misaligned addresses
    switch (value) {
    case HIGH:  // set digital output as HIGH
        *ec_slave[slaveNumber].outputs |= (1 << (moduleIndex - 1 + startbit));
        break;
    case LOW: // set digital output as LOW
        *ec_slave[slaveNumber].outputs &= ~(1 << (moduleIndex - 1 + startbit));
        break;
    }
}

uint8_t readDigitalInput (uint16 slaveNumber, uint8 moduleIndex)
{
   /* Get the the startbit position in slaves IO byte */
   uint8 startbit = ec_slave[slaveNumber].Istartbit;
   //cout << startbit << endl;
   /* Mask bit and return boolean 0 or 1 */
   if ( *ec_slave[slaveNumber].inputs & (1 << (moduleIndex - 1  + startbit)) )
      return 1;
   else
      return 0;
}

void testProgram(int* slaveTree) {
    while (true) {
        EL1008_device.in1 = readDigitalInput(EL1008, 1);
        cout << EL1008_device.in1 << endl;
    }
    for (int i = 1; i < LastSlave; i++) {
        cout << ec_slave[i].Ostartbit << endl;
        //setDigitalOutput(EL2008, 0, HIGH);
    }
}

/*
    build with 
    g++ beckhoff-on-soem.cpp -o beckhoff-on-soem -g `pkg-config --libs --cflags soem`

*/

int main(int argc, char* argv[]) { // argv[0] is the 
    // 1. initialize slave devices on the specified ethernet port
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

    if (!initSlaves(ethName, requestSlaveInfo)) { return 0; };

    // 2. set all devies to OP STATE prior running them
    if ( !requestAllSlavesToOPSTATE() ) { return 0; }; 

    // 3. run simple program
    testProgram(predefinedSlaveList);

    return 0;
}