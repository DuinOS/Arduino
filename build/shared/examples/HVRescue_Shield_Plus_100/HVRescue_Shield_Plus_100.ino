/*
 Title:        HVRescue_Shield_Plus
 Description:  Arduino sketch for use with the HV Rescue Shield
 Author:       Originally Jeff Keyzer, Modified and extended by Dennis Tricker
 Company:      MightyOhm Engineering 
 Website:      http://mightyohm.com
 Contact:      http://mightyohm.com/blog/contact/  
 
 This sketch assumes that the Arduino is equipped with an AVR HV Rescue Shield.
 Schematics and other details are available at http://mightyohm.com/hvrescue2
 
 The sketch uses High Voltage Programming Mode to set fuses on many Atmel AVR family 8-bit microcontrollers.
 Version 2.0 adds support for High Voltage Serial Programming (HVSP) mode and 8-pin ATtiny devices, but remains
 backwards compatible with the 1.x series hardware.
 
 The HVPP routines are based on those described in the ATmega48/88/168 datasheet rev.
 2545M-AVR-09/07, pg. 290-297 and the ATtiny2313 datasheet rev. 2543I-AVR-04/06 pg. 165-176.  
 
 The HVSP routines are based on the ATtiny25/45/85 and 13A datasheets (ATtiny25/45/85 2586M–AVR–07/10 pg. 159-165,
 ATtiny13A 8126E-AVR-7/10 pg. 109-116).
 
 These routines are compatible with many other members of the AVR family that are not listed here.
 For a complete list of tested microcontrollers, see http://mightyohm.com/wiki/products:hvrescue:compatibility
 
 Changelog:
 12/1/11 Plus 1.00 Rewritten to simplify device access and extend functionality, by Dennis J Tricker
             Added unified HVPP & SP low level code: target_xxxxxx functions & associated support routines (Load & Strobe)
             Introduced choice menu function and get_value with error checking and automation
             Fuse write now checked and indicated with OK or Error
             Extended chip data output to include SIG and LOCK bits
             Extended commands to allow Erase, FLASH & EEPROM Read 
             Added FLASH & EEPROM test page write: writes and checks pattern to ensure chip operational             
                 Note: Address must be a start of page address ! See datasheet
                 EEPROM test writes four bytes 0xaa,0x55,0xa5,0x5a
                 FLASH Test writes "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPp" = 32 bytes= 16 words. Upper case is MSB.
                 EEPROM&FLASH Test may not write a whole page due to limits on Arduino RAM memory, only check bytes written
             All output in hex, all input in hex: 0x prefix optional
             IMPORTANT: The Erase and Test functions will clear / overwrite existing device data - You have been warned !
             Tested with Arduino 22 & 1.0, ATtiny85/ATtiny2313/ATmega48

 3/15/11 2.12
 - New digital pin 0-7 (command&data) read/write routines for the Arduino Mega, since these lines are implemented
 differently on the Mega than on the original Arduino.
 
 3/8/11 2.11
 - Analog inputs (used here as digital outputs) are now called by their new Arduino A0-A5 names.
 
 2/2/11 2.1
 - adjusted RESET and VCC edge timing to work with new board design and avoid signal contention on SDO
 - fixed bug that prevented program from compiling in non-interactive mode
 - modified non-interactive mode so that read/verify serial comms still occur, but fuse values aren't prompted
 
 12/17/10 2.01
 - added missing braces to if(mode == HVSP) that sets SDO pinmode to INPUT
 - removed misleading comment about removing AVR when entering fuse values
 - default mode changed back to ATMEGA
 
 12/13/10 v2.0
 - Added support for 8-pin parts that use HV Serial Programming (HVSP)
 - New mode selection at startup determines which type of part is to be programmed 
 - Got rid of endSerial function, since Arduino now includes Serial.end (finally!)
 - Added a wait for serial transmit to complete before burning fuses.  Without this HFUSE burn would fail occasionally.
 - Numerous other minor tweaks, removal of unnecessary delays, better commenting
 
 9/24/10 v1.2a
 - ATtiny2313 mode was being set by default.  Changed default mode back to ATmega (see #define ATtiny).
 
 8/16/10 v1.2
 - Existing fuse settings are now shown before asking the user for new values
 - Added OE strobe after entering programming mode to get ATtiny2313 to read first fuse correctly.
 - Cleaned up code a bit
 - Some minor tweaks to data direction register settings during setup, etc.
 
 11/02/09 v1.1
 - Removed endSerial call after reading back fuse bytes, was spewing garbage into
 serial monitor
 - Still occsionally get garbage when opening serial monitor, not sure what is causing this.
 
 03/01/09 v1.0
 - ATtiny2313 support, enable with ATtiny option
 - 12V Step up converter enable is non-inverting, unlike previous level shifter circuit
 - added interactive mode, asks for fuse values to burn, option to turn off
 - added EFUSE support and option to enable
 - button now has very simple debounce routine
 
 09/24/08
 - original release of sketch "HVFuse" to support first implementation on perfboard
 - Details: http://mightyohm.com/blog/2008/09/arduino-based-avr-high-voltage-programmer/
 
 Original Copyright 2008, 2009, 2010 Jeff Keyzer
 Additions Copyright 2011 Dennis Tricker
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// User defined settings
#define  MEGA         0     // Set this to 1 if you are using an Arduino Mega (default = 0)
#define  MODE_SELECT  ASK   // Set this to ASK for mode select menu or ATMEGA, TINY2313, or HVSP for auto selection
#define  INTERACTIVE  1       // Set this to 0 to disable interactive (serial) mode and to always run F command (fuse write)
#define  BURN_EFUSE   1       // Set this to 1 to enable burning extended fuse byte
#define  BAUD         9600    // Serial port rate at which to talk to PC

// Old type hardware with Transistor switching 12v
//#define OFF12v  HIGH
//#define ON12v LOW

// New version 2.1 hardware with DC-DC convertor
#define OFF12v  LOW
#define ON12v HIGH

// If interactive mode is off, these fuse settings are used instead of user prompted values: Set as required
//#define  AUTO_LFUSE        0x62  // Factory default for ATmega168
//#define  AUTO_HFUSE        0xDF 
//#define  AUTO_EFUSE        0xF9  

#define  AUTO_LFUSE        0x64  // Factory default for ATtiny2312
#define  AUTO_HFUSE        0xDF  // 
#define  AUTO_EFUSE        0xFF  // 

//#define  AUTO_LFUSE        0x62  // Factory default for ATtiny85
//#define  AUTO_HFUSE        0xDF  // 
//#define  AUTO_EFUSE        0xFF  // 

/*
  Data line assignments
 Fuse and command data for HVPP mode are sent using Arduino digital lines 0-7
 
 Arduino Uno and other original-style form factor boards:
 Digital Line 0-7 outputs = PORTD
 Inputs = PIND
 Data direction register = DDRD
 
 Arduino Mega - much more complicated because digital lines 0-7 don't map directly to an AVR hardware port:
 Digital Line  AVR Signal Name
 0  PE0  (PORTE)
 1  PE1  (PORTE)  
 2  PE4  (PORTE)
 3  PE5  (PORTE)
 4  PG5  (PORTG)
 5  PE3  (PORTE) 
 6  PH3  (PORTH)
 7  PH4  (PORTH)
 */

// Pin Assignments (you shouldn't need to change these) 
#define  VCC     12
#define  RDY     13     // RDY/!BSY signal from target
#define  OE      11
#define  WR      10
#define  BS1     A2
#define  XA0     8
#define  XA1     A4    
#define  RST     A0    // 12V Step up converter enable (12V_EN)
#define  XTAL1   A3
#define  BUTTON  A1    // Run button

// Pin assignments for HVSP mode 
#define  SCI    BS1
#define  SDO    RDY
#define  SII    XA0
#define  SDI    XA1


// Commands for HV prog mode, used for both PP and SP, comment shows usage
#define HV_CMD_CHIP_ERASE     B10000000 // CMD *WR
#define HV_CMD_WRITE_FUSE     B01000000 // CMD DATAL *WR
#define HV_CMD_WRITE_LOCK     B00100000 // CMD DATAL *WR
#define HV_CMD_WRITE_FLASH    B00010000 //  CMD ({ADDRL DATAL DATAH *PG} ADDRH *WR) NOP
#define HV_CMD_WRITE_EEPROM   B00010001 //  CMD ADDRH {ADDRL DATAL *PG} *WR // HVSP shows {ADDRL ADDRH but other seems ok
#define HV_CMD_READ_SIG       B00001000 // CMD ADDRL *RD
#define HV_CMD_READ_FUSE_LOCK B00000100 // CMD *RD
#define HV_CMD_READ_FLASH     B00000010 // CMD ADDRH ADDRL *RD // HVSP shows L first but H seems only
#define HV_CMD_READ_EEPROM    B00000011 // CMD ADDRH ADDRL *RD // HVSP shows L first but H seems only

// Load type bitmap for HV prog: (0 0 0 0 0) XA1 XA0 BS1
//  Used as is for HVSP, HVPP decodes bits to setup ports
#define LOAD_ADDRESS   B00000000
#define LOAD_ADDRESS_H B00000001
#define LOAD_DATA      B00000010
#define LOAD_DATA_H    B00000011
#define LOAD_COMMAND   B00000100
#define POST_OP        B00000110 // Only used for HVSP

// Post-op bitmap for HV Prog: (0 0 0) BS1 WE OE BS2 PAGEL
//  Used as is for HVSP, HVPP decodes bits to setup ports
#define LFUSE_SEL_R     B00001000
#define LFUSE_SEL_W     B00000100
#define HFUSE_SEL_R     B00011010 
#define HFUSE_SEL_W     B00010100
#define EFUSE_SEL_R     B00001010
#define EFUSE_SEL_W     B00000110
#define LOCK_SEL_R      B00010000
#define SIG_SEL_R       B00001000
#define LSB_R           B00001000
#define MSB_R           B00011000
#define SEL_WR          B00000100
#define EE_PAGE_LATCH   B00001101
#define FL_PAGE_LATCH   B00011101
#define MASK_WE_OE      B00001100 // Make Write and Output enable strobes inactive
#define MASK_BS1        B00010000
#define MASK_BS2        B00000010
#define MASK_WE         B00001000
#define MASK_OE         B00000100
#define MASK_PAGEL      B00000001

// Serial instructions for HVSP mode
// Based on the ATtiny85 datasheet Table 20-16 pg. 163-165.
// After analysis serial instructions can be built from the parallel ones

// Internal definitions
#define ASK           0 // Note: it is 0 not '0' for no auto selection
#define ATMEGA      '1'
#define TINY2313    '2'
#define HVSP        '3'
enum result_types { 
    DONEDEFAULT, SUCCESS, FAILURE };

// Global variables
byte mode;  // programming mode
byte burn_efuse = BURN_EFUSE; // now done at run time
byte tbuffer[64];
byte test_ee[]={0xaa,0x55,0xa5,0x5a}; // Four bytes
byte test_fl[]="AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPp"; // 16 bytes, 8 words. Upper case is MSB

// The mode menu
const char* menu_m[]={
    "\nSelect mode:",
    "1: ATmega (28-pin)",
    "2: ATtiny2313/4313",
    "3: ATtiny (8-pin) / HVSP",
    0}; // end

// The function menu
const char* menu_f[]={
    "\nSelect function:",
    "E: Erase",
    "F: Write Fuses",
    "R: Read Flash",
    "P: Read EEPROM",
    "W: Test Flash",
    "T: Test EEPROM",
    "S: Safely Remove",
    0}; // end

byte config[16]; // Used to hold data read from target
#define SIG1     0
#define SIG2     1
#define SIG3     2
#define LFUSE    3
#define HFUSE    4
#define EFUSE    5
#define LOCK     6

// These pin assignments change depending on which chip is being programmed,
// so they can't be set using #define
// There is probably a more elegant way to do this.  Suggestions?
byte PAGEL = A5;  // ATtiny2313: PAGEL = BS1
byte BS2 = 9;     // ATtiny2313: BS2 = XA1

void setup() { // run once, when the sketch starts

    // Initialize most important hardware, the rest waits til after target type is known
    pinMode(VCC, OUTPUT);  // Target Supply
    pinMode(RST, OUTPUT);  // Control of DC-DC converter that generates +12V !RESET
    digitalWrite(RST, OFF12v);  // Turn off 12V, Reset=0
    
    digitalWrite(VCC, LOW); // DJT: OK if no chip inserted
    //digitalWrite(VCC, HIGH); // DJT: Alternative if chip inserted already
    
    pinMode(BUTTON, INPUT); // User button to start programming
    digitalWrite(BUTTON, HIGH);  // turn on internal pullup resistor

    Serial.begin(BAUD);  // Open serial port, this works on the Mega also because we are using serial port 0
    delay(100); 

    Serial.println("\n*** AVR HV Rescue Plus v1.0"); 
    // Ask user which chip family we are programming
    mode=*get_choice(menu_m, MODE_SELECT);

    // Setup AVR interface
    if(mode == TINY2313){
        PAGEL = BS1; // reassign PAGEL and BS2 to their combined counterparts on the '2313
        BS2 = XA1;
    } 
    target_exit_program_mode();
}

void loop() {  // run over and over again

    byte hfuse=0xff, lfuse=0xff, efuse=0xff;  // desired fuse values from user
    char cmd; 
    static char allow_removal=true;
    int start_addr=0,i;
    byte function_result;

    if (allow_removal){
        Serial.println("\nInsert device and press button ...");
        Serial.end();

        if(mode != HVSP) 
            data_as_output(0);    // Set Data low, not good for TX&RX but prevents them powering target

        // wait for button press, debounce
        while(1) {
            while (digitalRead(BUTTON) == HIGH);  // wait here until button is pressed
            delay(100);                            // simple debounce routine
            if (digitalRead(BUTTON) == LOW)       // if the button is still pressed, continue
                break;  // valid press was detected, continue on with rest of program
        }
    }
    else
        Serial.end();

    // Put target device into program mode
    target_enter_program_mode();

    /****
     **** Now we're in programming mode until RST is set LOW again
     ****/

    // Read the Signature
    config[SIG1] = target_io(HV_CMD_READ_SIG,0,0,SIG_SEL_R);
    config[SIG2] = target_io(HV_CMD_READ_SIG,1,0,SIG_SEL_R);
    config[SIG3] = target_io(HV_CMD_READ_SIG,2,0,SIG_SEL_R);

    // Get current fuse settings stored on target device 
    config[LFUSE] = target_io(HV_CMD_READ_FUSE_LOCK,0,0,LFUSE_SEL_R);
    config[HFUSE] = target_io(HV_CMD_READ_FUSE_LOCK,0,0,HFUSE_SEL_R);
    if (burn_efuse)
        config[EFUSE] = target_io(HV_CMD_READ_FUSE_LOCK,0,0,EFUSE_SEL_R);
    config[LOCK] = target_io(HV_CMD_READ_FUSE_LOCK,0,0,LOCK_SEL_R);

    // Open serial port again to print existing values
    Serial.begin(BAUD);
    Serial.print("\n*** Current Chip Data");  
    printf("\n Signature:      % % %", &config[SIG1]);
    if (burn_efuse)
        printf("\n Fuses Lo/Hi/Ex: % % %", &config[LFUSE]);
    else
        printf("\n Fuses Lo/Hi:    % %", &config[LFUSE]);
    printf("\n Lock Bits:      %", &config[LOCK]);
    Serial.print("\n");  

#if (INTERACTIVE == 1)
    // Get function required
    cmd=*get_choice(menu_f,0);

    // Get Additional data
    if (cmd=='R' || cmd=='P' || cmd=='T'|| cmd=='W')
        start_addr=get_value("Address",0x00);
    else if (cmd=='F'){ 
        // Ask the user what fuse values should be burned to the target
        lfuse = get_value("LFUSE value",0x62);
        hfuse = get_value("HFUSE value",0xdf); 
        if (burn_efuse)
            efuse = get_value("EFUSE value",0xff);
    }
    allow_removal=false; // Allow multiple commands without button press

#else  // not using interactive mode, just set fuses to values defined in header
    // Auto select function required
    start_addr=0;
    cmd=*get_choice(menu_f,'F');
    hfuse = AUTO_HFUSE;
    lfuse = AUTO_LFUSE;
    efuse = AUTO_EFUSE;
    allow_removal=true; // Always allow chip removal
#endif

    // This business with TXC0 is required because Arduino doesn't give us a means to tell if a serial 
    // transmission is complete before we move on and do other things.  If we don't wait for TXC0 to be reset,
    // I found that sometimes the 1st fuse burn would fail.  It turns out that DATA1 (which doubles as Arduino serial
    // TX) was still toggling by the time the 1st XTAL strobe latches the fuse program command.  Bad news.
    //UCSR0A |= _BV(TXC0);  // Reset serial transmit complete flag (need to do this manually because TX interrupts aren't used by Arduino)
    //while(!(UCSR0A & _BV(TXC0)));  // Wait for serial transmission to complete before burning fuses!
    // The above will no longer work with Arduino 1.0 which buffers transmit data
    Serial.flush(); // In Arduino 1.0 this will wait for transmission to finish (prior to that will empty receive buffer so we reply on the delay after serial end to ensure TX finished)
    Serial.end();    // We're done with serial comms (for now) so disable UART
    delay(200);

    // Now run selected function
    function_result=DONEDEFAULT; // Default response to 'done'
    switch(cmd){
    case 'E': // Chip Erase
        target_io(HV_CMD_CHIP_ERASE,0,0,SEL_WR); // Perform a chip erase
        break;
    case 'F': // Write fuses with user values
        target_io(HV_CMD_WRITE_FUSE,0,lfuse,LFUSE_SEL_W);
        target_io(HV_CMD_WRITE_FUSE,0,hfuse,HFUSE_SEL_W);
        if (burn_efuse)
            target_io(HV_CMD_WRITE_FUSE, 0, efuse, EFUSE_SEL_W);

        // Read back fuse contents to verify burn worked 
        config[LFUSE] = target_io(HV_CMD_READ_FUSE_LOCK,0,0,LFUSE_SEL_R);
        config[HFUSE] = target_io(HV_CMD_READ_FUSE_LOCK,0,0,HFUSE_SEL_R);
        if (burn_efuse)
            config[EFUSE]  = target_io(HV_CMD_READ_FUSE_LOCK,0,0,EFUSE_SEL_R);

        // Check result
        if (config[LFUSE]!=lfuse || config[HFUSE]!=hfuse || ( burn_efuse && (config[EFUSE]!=efuse)))
            function_result=FAILURE;        
        else        
            function_result=SUCCESS;
        break;       
    case 'R': // Read and print block of FLASH
        for (i=0;i<64;i+=8)
            target_dump("FLASH", HV_CMD_READ_FLASH, start_addr+i, 16); // Read 16 bytes = 8 words
        break;
    case 'P': // Read and print block of EEPROM 
        for (i=0;i<128;i+=16)
            target_dump("EEPROM", HV_CMD_READ_EEPROM, start_addr+i, 16);
        break;
    case 'S': // Safely remove chip from socket
        allow_removal=true;
        break;
    case 'W': // Test write FLASH
        target_page_write(HV_CMD_WRITE_FLASH, start_addr, test_fl, 16); // Write 16 bytes = 8 words, must be equal or less than device page size. Arduino ram limits prevent us writing a whole page for now
        function_result=SUCCESS; // Assume it worked unless test show otherwise
        for (i=0;i<8;i++){
            if ((test_fl[i*2]*256+test_fl[i*2+1])!=(target_io(HV_CMD_READ_FLASH,  start_addr+i, 0, MSB_R)*256+target_io(HV_CMD_READ_FLASH,  start_addr+i, 0, LSB_R)))
                function_result=FAILURE;       
        }
        break;
    case 'T': // Test write EEPROM
        target_page_write(HV_CMD_WRITE_EEPROM, start_addr, test_ee, 4); // Use EEPROM page size of 4 (Correct for all AVR we are likely to see
        function_result=SUCCESS; // Assume it worked unless test show otherwise
        for (i=0;i<4;i++){
            if (test_ee[i]!=target_io(HV_CMD_READ_EEPROM,  start_addr+i, 0, LSB_R))
                function_result=FAILURE;       
        }
        break;
    }

    Serial.begin(BAUD);  // open serial port
    // Print result
    Serial.write('\n');
    if (function_result==DONEDEFAULT)
        Serial.println(" Done"); // Non commital result where functions doesn't check (default)
    if (function_result==SUCCESS)
        Serial.println(" OK"); // Success
    if (function_result==FAILURE)
        Serial.println(" Fail"); // Failure
    target_exit_program_mode(); // Place after serial io, so Vcc is still on and prevents corruption
}

void target_dump(const char* title, byte command, int address, int len){
    int i=0,n;
    n=address;
    do{
        if (command==HV_CMD_READ_FLASH)
            tbuffer[i++]=target_io(command, n, 0, MSB_R); //Byte buffer order MSB-LSB MSB-LSB .....
        tbuffer[i++]=target_io(command, n++, 0, LSB_R);
    }   
    while (i<len);    
    delay(10);
    Serial.begin(BAUD);  // open serial port
    delay(10);

    Serial.write('\n');
    Serial.print(title);
    print_fixed_hex(highByte(address));
    print_fixed_hex(lowByte(address));
    i=0;
    do {
        Serial.write(' ');
        if (command==HV_CMD_READ_FLASH)
            print_fixed_hex(tbuffer[i++]); // MSB
        print_fixed_hex(tbuffer[i++]);
    }
    while (i<len);    
    Serial.write('\n');
    delay(10);
    Serial.end();
    delay(10);
}


void target_enter_program_mode(void){

    // Initialize pins to enter programming mode
    digitalWrite(RST, OFF12v);   // Reset must be LOW
    digitalWrite(VCC, LOW); // VCC must be off

    if(mode == HVSP) {
        // Set necessary pin values to enter programming mode
        pinMode(SDI, OUTPUT);   
        digitalWrite(SDI, LOW);  
        pinMode(SII, OUTPUT);   
        digitalWrite(SII, LOW);
        pinMode(SDO, OUTPUT);   // normally an output but
        digitalWrite(SDO, LOW);  // needs to be low to enter programming mode
        // Setup other pins needed during prog
        pinMode(SCI, OUTPUT);   
        digitalWrite(SCI, LOW);  // set clock low
    }
    else{
        // Set necessary pin values to enter programming mode
        pinMode(PAGEL, OUTPUT);
        digitalWrite(PAGEL, LOW);
        pinMode(XA1, OUTPUT);
        digitalWrite(XA1, LOW);
        pinMode(XA0, OUTPUT);
        digitalWrite(XA0, LOW);
        pinMode(BS1, OUTPUT);
        digitalWrite(BS1, LOW);
        pinMode(WR, OUTPUT);
        digitalWrite(WR, LOW);  // ATtiny2313 needs this to be low to enter programming mode, ATmega doesn't care

        // Setup other pins needed during prog
        data_as_input();
        pinMode(RDY, INPUT);
        pinMode(XTAL1, OUTPUT);           
        digitalWrite(XTAL1, LOW);
        pinMode(BS2, OUTPUT);
        digitalWrite(BS2, LOW);
        pinMode(OE, OUTPUT);
        digitalWrite(OE, LOW);
    }

    // Enter programming mode
    digitalWrite(VCC, HIGH);  // Apply VCC to start programming process
    delayMicroseconds(80);
    digitalWrite(RST, ON12v);   // Apply 12V to !RESET thru level shifter

    if(mode == HVSP) {
        // reset SDO after short delay, longer leads to logic contention because target sets SDO high after entering programming mode
        delayMicroseconds(1);   // datasheet says 10us, 1us is needed to avoid drive contention on SDO 
        pinMode(SDO, INPUT);    // set to input to avoid logic contention
    }
    else{
        delayMicroseconds(10);  // Give lots of time for part to enter programming mode
        digitalWrite(OE, HIGH);
        digitalWrite(WR, HIGH);   // Now that we're in programming mode we can disable !WR
    }
    delay(1);
}

void target_exit_program_mode(void){
    // Exit programming mode
    digitalWrite(RST, OFF12v);  
    delay(1);

    // Turn off pins driving chip
    if(mode == HVSP) {
        pinMode(SDI, INPUT);   
        digitalWrite(SDI, LOW);   // with no pullup
        pinMode(SII, INPUT);   
        digitalWrite(SII, LOW);
        pinMode(SCI, INPUT);   
        digitalWrite(SII, LOW);
        //pinMode(SDO, INPUT);   // is an input anyway
    }
    else{    
        data_as_input(); // Should be input anyway
        digitalWrite(OE, LOW);
        digitalWrite(WR, LOW);
        digitalWrite(PAGEL, LOW);
        digitalWrite(XA1, LOW);
        digitalWrite(XA0, LOW);
        digitalWrite(BS1, LOW);
        digitalWrite(BS2, LOW);
    }
    digitalWrite(VCC, LOW); // Turn off power
}

void load(byte type, byte data){ // Load Commands and data into target

    if (mode != HVSP) { // HVPP programming only
        // Set controls for requested transaction type
        digitalWrite(BS2, LOW);  // Not relevant for load but should be low, must set first as shared on tinyX313
        digitalWrite(XA1, b2hilo(type & 0x04)); 
        digitalWrite(XA0, b2hilo(type & 0x02)); 
        digitalWrite(BS1, b2hilo(type & 0x01)); 

        // Output data
        delay(1);
        data_as_output(data);
        strobe_xtal();  // latch DATA
        data_as_input(); // reset DATA to input to avoid bus contentions
    }
    else
    {
        HVSP_sio(data, ((type<<4) +12));
    }
}

byte strobe(byte select){ // Do a WE or OE or PAGEL strobe, returns value on OE
    byte value=0;    
    if (mode != HVSP) { // HVPP programming only

        // Activate Strobe, Made complicated by fact BS2 & PAGEL shared on Tiny2313
        if (select & MASK_PAGEL){
            digitalWrite(WR, HIGH );  // Setup WR
            digitalWrite(OE, HIGH );  // Setup OE          
            digitalWrite(BS1, LOW );  // Setup BS1
            digitalWrite(BS2, LOW );  // Setup BS2          
            digitalWrite(PAGEL, HIGH);  // Setup PAGEL
        }
        else{ // WR or OE use BS1 & BS2
            digitalWrite(PAGEL, LOW);  // PAGEL inactive
            digitalWrite(BS1, b2hilo(select & MASK_BS1) );  // Setup BS1
            digitalWrite(BS2, b2hilo(select & MASK_BS2) );  // Setup BS2
            delay(1);
            // Now make required strobe active
            digitalWrite(WR, b2hilo(select & MASK_WE) );  // Setup WE, note active low
            digitalWrite(OE, b2hilo(select & MASK_OE) );  // Setup OE, note active low
        }
        delay(1);

        // Capture data on reads
        if ((select & MASK_OE)==0)
            value=data_read();  // read value  

        // Now make all strobes inactive
        digitalWrite(WR,HIGH);
        digitalWrite(OE,HIGH);
        digitalWrite(PAGEL,LOW); 

        // Wait for writes to complete
        if ((select & MASK_WE)==0){
            while(digitalRead(RDY) == LOW);  // when RDY goes high, burn is done
        }   
    }
    else
    { // HVSP programming only, uses same serial protocol as load
        HVSP_sio(0, ((POST_OP<<4)|select)); // select will contain appropriate WE or OE or PAGEL setting
        value=HVSP_sio(0, ((POST_OP<<4) | select | MASK_WE_OE) & (~MASK_PAGEL)); // Get result, ensure strobes turned off
        // Wait for writes to complete
        if ((select & MASK_WE)==0){
            while(digitalRead(SDO) == LOW); // Wait for operation to complete
        }
    }
    return value;    
}

// Carry out a transaction with the target, all except Flash & EEPROM write because they are paged
byte target_io(byte command, int address, byte data, byte select) {
    byte value;

    // Load the command and parameters into target
    load(LOAD_COMMAND,command);  // Send command
    if (command & (HV_CMD_READ_FLASH|HV_CMD_READ_EEPROM))
        load(LOAD_ADDRESS_H,highByte(address)); // Load MSB address if required into target
    if (command & (HV_CMD_READ_SIG|HV_CMD_READ_FLASH|HV_CMD_READ_EEPROM))
        load(LOAD_ADDRESS,lowByte(address)); // Load LSB address if required into target
    if (command & (HV_CMD_WRITE_FUSE|HV_CMD_WRITE_LOCK|HV_CMD_WRITE_FLASH|HV_CMD_WRITE_EEPROM))
        load(LOAD_DATA,lowByte(data)); // Load LSB value into target

    // Now do strobe to carry out operation    
    value=strobe(select);
    return value;
}

// Carry out a page transaction with the target chip.
//   address MUST be the start of a page block
void target_page_write(byte command, int address, byte* data, byte count) {
    byte i;

    // Load the command and parameters into target
    load(LOAD_COMMAND,command);  // Send command
    if (command==HV_CMD_WRITE_EEPROM){
        load(LOAD_ADDRESS_H,highByte(address)); // Load MSB address   
        for (i=0;i<count;i++){
            load(LOAD_ADDRESS,lowByte(address)); // Load LSB address 
            load(LOAD_DATA,lowByte(*data++)); // Load LSB value into target
            strobe(EE_PAGE_LATCH);
            address++;
        }
    }
    else{
        load(LOAD_ADDRESS_H,highByte(address)); // Load MSB address   
        for (i=0;i<count;i++){
            load(LOAD_ADDRESS,lowByte(address)); // Load LSB address 
            load(LOAD_DATA,lowByte(*(data+1))); // Load LSB (Byte buffer order MSB-LSB ... MSB-LSB)
            load(LOAD_DATA_H,lowByte(*data)); // Load MSB value into target
            data+=2; // Increment to next word
            strobe(EE_PAGE_LATCH | 0x10);
            address++;
        }
    }
    // Now do WR strobe to carry out operation    
    strobe(SEL_WR);
    load(LOAD_COMMAND,0);  // Send NOP command as per some datasheets 
}

// Notes on Page writing: 
//    HVSP EEPROM seems ok to write AddrH first and only once per page
//    Tiny2313 doesn't need PAGE_LATCH pulse but is ok if it is used
//    OK to write FLASH AddrH before loading page
//    HVSP PAGEL needs BS1 (?BS2) to be set 

byte HVSP_sio(byte datab, byte instrb) { // Do a Data & Instruction transfer and capture a byte using the HVSP protocol 
    byte response = 0x00; // the response from target
    int data,instr; //       0 XA1 XA0 BS1   WE OE BS2 (PAGE)

    data= (int)datab<<7; // Pad required bits with leading 0 and 2 trailing 0 as per prog spec
    instr= (int)instrb<<7;

    // We capture a response on every read even though only certain responses contain
    // valid data.  For fuses, the valid response is captured on the 3rd instruction write.
    // It is up to the program calling this function to figure out which response is valid.

    // The MSB of the response byte comes "early", that is, 
    // before the 1st non-zero-padded byte of the 3rd instruction is sent to the target.
    // For more information, see the ATtiny25/45/85 datasheet, Table 20-16 (pg. 164).

    // Send each bit of padded data and instruction byte serially, MSB first
    for (int i=0; i<11; i++) {  // i is bit number
        digitalWrite(SDI, int2hilo(data & 0x8000)); // Set SDI to represent next bit in data
        data<<=1;
        digitalWrite(SII, int2hilo(instr & 0x8000));  // Set SII to represent next bit in instruction
        instr<<=1;
        sclk();

        if (i < 8) {  // remaining 7 bits of response are read here (one at a time)
            if(digitalRead(SDO) == HIGH)  // if we get a logic 1 from target,
                response |= (0x80 >> i);    // set corresponding bit of response to 1
        }
    }
    return response;
}


#if (MEGA == 1)  // functions specifically for the Arduino Mega

void mega_data_write(byte data) { // Write a byte to digital lines 0-7
    // This is really ugly, thanks to the way that digital lines 0-7 are implemented on the Mega.
    PORTE &= ~(_BV(PE0) | _BV(PE1) | _BV(PE4) | _BV(PE5) | _BV(PE3));  // clear bits associated with digital pins 0-1, 2-3, 5
    PORTE |= (data & 0x03);  // set lower 2 bits corresponding to digital pins 0-1
    PORTE |= (data & 0x0C) << 2;  // set PORTE bits 4-5, corresponding to digital pins 2-3
    PORTE |= (data & 0x20) >> 2;  // set PORTE bit 5, corresponding to digital pin 5
    DDRE |= (_BV(PE0) | _BV(PE1) | _BV(PE4) | _BV(PE5) | _BV(PE3));  // set bits we are actually using to outputs

    PORTG &= ~(_BV(PG5));  // clear bits associated with digital pins 4-5
    PORTG |= (data & 0x10) << 1;  // set PORTG bit 5, corresponding to digital pin 4
    DDRG |= (_BV(PG5));  // set to output

    PORTH &= ~(_BV(PH3) | _BV(PH4));  // clear bites associated with digital pins 6-7
    PORTH |= (data & 0xC0) >> 3;  // set PORTH bits 3-4, corresponding with digital pins 6-7
    DDRH |= (_BV(PH3) | _BV(PH4));  // set bits to outputs
}

byte mega_data_read(void) { // Read a byte from digital lines 0-7
    byte data = 0x00;  // initialize to zero
    data |= (PINE & 0x03);  // set lower 2 bits
    data |= (PINE & 0x30) >> 2;  // set bits 3-4 from PINE bits 4-5
    data |= (PINE & 0x08) << 2;  // set bit 5 from PINE bit 3
    data |= (PING & 0x20) >> 1;  // set bit 4 from PING bit 5
    data |= (PINH & 0x18) << 3;  // set bits 6-7 from PINH bits 3-4
    return data;
}

void mega_data_input(void) { // Set digital lines 0-7 to inputs and turn off pullups
    PORTE &= ~(_BV(PE0) | _BV(PE1) | _BV(PE4) | _BV(PE5) | _BV(PE3));  // Mega digital pins 0-3, 5
    DDRE &= ~(_BV(PE0) | _BV(PE1) | _BV(PE4) | _BV(PE5) | _BV(PE3));  // Set to input
    PORTG &= ~(_BV(PG5));  // Mega digital pin 4
    DDRG &= ~(_BV(PG5));  // Set to input
    PORTH &= ~(_BV(PH3) | _BV(PH4));  // Mega digital pins 6-7
    DDRH &= ~(_BV(PH3) | _BV(PH4));  // Set to input
}
#endif

void sclk(void) {  // send serial clock pulse, used by HVSP commands
    // These delays are much  longer than the minimum requirements, but we don't really care about speed.
    delayMicroseconds(100);  
    digitalWrite(SCI, HIGH);
    delayMicroseconds(100);
    digitalWrite(SCI, LOW);
}

void strobe_xtal(void) {  // strobe xtal (usually to latch data on the bus)
    delay(1);  
    digitalWrite(XTAL1, HIGH);  // pulse XTAL to send command to target
    delay(1);  
    digitalWrite(XTAL1, LOW);
}

void data_as_input(void) { // Set the data port as an input
#if (MEGA == 0)  // Set up data lines on original Arduino
    PORTD = 0x00;  // clear digital pins 0-7
    DDRD = 0x00;  // set digital pins 0-7 as inputs for now
#else  // Mega uses different ports for same pins, much more complicated setup.  yuck!
    mega_data_input();
#endif
}

void data_as_output(byte data){ // Set the data port as an output with data
#if (MEGA == 0)
    PORTD = data;
    DDRD = 0xFF;  // Set all DATA lines to outputs
#else
    mega_data_write(data);
#endif
}

byte data_read(void) { // Capture value on data port
    byte value;
#if (MEGA == 0)
    value = PIND;
#else
    value = mega_data_read();
#endif
    return value;
}

byte int2hilo(int n) // Converts an int to HIGH or LOW
{
    if (n)
        return HIGH;
    return LOW;
}

byte b2hilo(byte n) // Converts a byte to HIGH or LOW
{
    if (n)
        return HIGH;
    return LOW;
}

int hex2dec(byte c) { // converts one HEX character into a number
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    else if (c >= 'a' && c <= 'f') { // Added to allow lower case hex
        return c - 'a' + 10;
    }
    return -1;
}

// Simple printf substitute
void printf(const char* format, byte* param){ // Format string usings % to print 2 digit hex from param array
    char c;    
    while ((bool)(c=*format++)){
        if (c=='%'){
            if (*param<0x10)
                Serial.print("0"); 
            Serial.print(*param++, HEX);
        }
        else
            Serial.write(c);
    }
}

void print_fixed_hex(int value){ // Prints a hex value as two fixed digits
    //Serial.print(" ");
    if (value<0x10)
        Serial.print("0"); 
    Serial.print(value, HEX);
}

// Get hex value from the user, handles upper and lower case & errors. Optional 0x prefix ignored
int get_value(const char* prompt, int def ) {  // def parameter not yet used, could provide default input
    int incomingByte;
    int n,m,v=0;
    byte err;

    // Prompt
    Serial.print(prompt);

    do {
       delay(100);
       while (Serial.available())   // Clear buffer of any input ie flush
            Serial.read();
        // Serial.flush(); // Not supported in Arduino 1.0
        Serial.print(" ?");

        // Wait for input
        do
            n=Serial.available();
        while (n==0);

        // Wait for input to finish (timeout) . Allow variable length input without terminator
        do {
            delay(100);
            m=n;
            n=Serial.available();
        }
        while (m!=n); 

        // Process characters recieved
        err=0; //Assume no errors in input
        while (!err && ((incomingByte = Serial.read()) != -1)){ 
            if  (incomingByte=='\n' )
                break;
            if  (incomingByte=='\r' )
                break;        
            if  ((v==0) && (incomingByte=='0') && (Serial.peek()=='x'))
                incomingByte = Serial.read();    // Ignore 0x prefix at start only
            else if  (hex2dec(incomingByte)==-1)
                err=1;     
            else
                v=v*16 + hex2dec(incomingByte);
        }
    }
    while(err); // Repeat until no errors 

    Serial.println(v, HEX);  // echo value back to the user
    return v;
}

// Print a simply menu and get user selection of option
//    options = Array of strings, [0] is the intro, others prompt with selection char first
//    auto = 0 if interactive mode, else char to use as input in non-interactive mode
const char* get_choice(const char* options[], char auto_cmd){ 
    int i=0,j,k;
    char c;

    Serial.println(options[0]); // First item is into
    while(options[++i]!=0){
        Serial.write(' '); // Indent
        Serial.println(options[i]); // Print each option
    }

    k=0;  // Will be set to index of option selected  
    do {
        while (Serial.available())   // Clear buffer of any imput ie flush
            Serial.read();
        // Serial.flush(); // Not supported in Arduino 1.0

        if (k==0)
            Serial.write('?'); // Prompt the user if no choice made yet
            
        if (auto_cmd){
            c=auto_cmd; // Use the auto input
            auto_cmd=0; //  but only first time
        } 
        else {       
            while (Serial.available() == 0)delay(10);   // wait for a character to come in
            c=toUpperCase(Serial.read());
            Serial.write(c);
        }

        // Search to see if entry valid
        for (j=1;j<i;j++)
            if (c==*options[j])
                k=j;
            else
                Serial.write(*options[j]);
    }
    while (k==0);

    Serial.write(c); // echo user input
    Serial.write('\n');
    Serial.print(options[k]+2); // Print chosen option
    Serial.println(" selected");
    return (options[k]); // Return pointer to selected option
}


































