const int NUMBER_OF_SLAVES = 3; // can be changed down the line
enum SlaveTree { 
    tree_EK1100 = 1, 
    tree_EL1008 = 2, 
    tree_EL2008 = 3, 
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
} EL1008;

static uint8_t el1008_mask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}; // lsb --> msb

typedef struct {
   uint8_t  out1;
   uint8_t  out2;
   uint8_t  out3;
   uint8_t  out4;
   uint8_t  out5;
   uint8_t  out6;
   uint8_t  out7;
   uint8_t  out8;
} EL2008;

