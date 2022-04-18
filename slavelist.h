const int NUMBER_OF_SLAVES = 3; // can be changed down the line
enum SlaveTree { 
    EK1100 = 1, 
    EL1008 = 2, 
    EL2008 = 3, 
    LastSlave 
};

typedef struct {
   uint8_t  in1;
   uint8_t  in2;
   uint8_t  in3;
   uint8_t  in4;
   uint8_t  in5;
   uint8_t  in6;
   uint8_t  in7;
   uint8_t  in8;
} EL1008_slave;