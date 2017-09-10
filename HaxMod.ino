//This section is for user inputted data to make the code personalized to the particular controller

//shield drop
#define sw_notch_x_value -.7000
#define sw_notch_y_value -.7000
#define se_notch_x_value  .7000
#define se_notch_y_value -.7000
//record southwest and southeast notch values

//to switch to dolphin mode hold dpad right for 5 seconds
//to return to a stock controller hold dpad left for 10 seconds

//======================================================================================================================
//  Install Intstructions:
//    1. connect arduino to computer and make sure proper board [Arduino Nano], processor [ATmega328], and COM
//       port [highest number] are selected under Tools.
//    2. hit the upload button (the arrow) and wait for it to say "Done Uploading" at the bottom before disconnecting
//       the Arduino.
//    3. connect 5V and Gnd from GCC header to 5V and Gnd pins on arduino (do not sever either wire)
//    4. sever data wire between GCC cable and header on controller
//    5. connect D2 on Arduino to data header on controller, and D3 to data wire from cable
//    6. ensure no wires are caught anywhere and close everything up
//
//    Note: if there is any trouble with steps 1 or 2, get the CH340 driver for your operating system from google
//======================================================================================================================





#include "Nintendo.h"

// Sets D2 on Arduino to read data from controller
CGamecubeController controller(2);

// Sets D3 on Arduino to write data to console
CGamecubeConsole console(3);

// Structural values for controller state
Gamecube_Report_t gcc;
bool shield, tilt, dolphin = 0, off = 0;
byte axm, aym, cxm, cym, cycles;
char ax, ay, cx, cy, buf;
float swang, seang;
word mode, toggle;
unsigned long n;

void convertinputs()
{
    // Reduces deadzone of cardinals and gives steepest/shallowest angles when on or near the gate
    perfectangles();

    // Snaps sufficiently high cardinal inputs to vectors of 1.0 magnitude of analog stick and c stick
    maxvectors();

    // Allows shield drops down and gives a 6 degree range of shield dropping centered on SW and SE gates
    shielddrops();

    // Fixes dashback by imposing a 1 frame buffer upon tilt turn values
    backdash();

    // Ensures close to 0 values are reported as 0 on the sticks to fix dolphin calibration and allows user to switch to dolphin mode for backdash
    dolphinfix();

    // Function to disable all code if DPAD LEFT is held for 10 seconds
    nocode();
}
//======================================================================================================================
// Reduce the dead zone in cardinal directions and give the steepest/shallowest angles when on or near the gate
//======================================================================================================================
void perfectangles()
{
    if(axm>75){gcc.xAxis = (ax>0)?204:52; if(aym<23) gcc.yAxis = (ay>0)?151:105;}
    if(aym>75){gcc.yAxis = (ay>0)?204:52; if(axm<23) gcc.xAxis = (ax>0)?151:105;}
}
//======================================================================================================================
// Snaps sufficiently high cardinal inputs to vectors of 1.0 magnitude of analog stick and c stick
//======================================================================================================================
void maxvectors()
{
    if(axm>75&&aym< 9){gcc.xAxis  = (ax>0)?255:1; gcc.yAxis  = 128;}
    if(aym>75&&axm< 9){gcc.yAxis  = (ay>0)?255:1; gcc.xAxis  = 128;}
    if(cxm>75&&cym<23){gcc.cxAxis = (cx>0)?255:1; gcc.cyAxis = 128;}
    if(cym>75&&cxm<23){gcc.cyAxis = (cy>0)?255:1; gcc.cxAxis = 128;}
}
//======================================================================================================================
//  Allows shield drops down and gives a 6 degree range of shield dropping centered on SW and SE gates
//======================================================================================================================
void shielddrops()
{
    shield = gcc.l||gcc.r||gcc.left>74||gcc.right>74||gcc.z;
    if(shield)
    {
        if(ay<0&&mag(ax,ay)>75)
        {

            if(ax<0&&abs(ang(axm,aym)-swang)<4)
            {
                gcc.yAxis = 73; gcc.xAxis =  73;
            }

            if(ax>0&&abs(ang(axm,aym)-seang)<4)
            {
                gcc.yAxis = 73; gcc.xAxis = 183;
            }
        }
        else if(abs(ay+39)<17&&axm<23) gcc.yAxis = 73;
    }
}
//======================================================================================================================
// Fixes dashback by imposing a 1 frame buffer upon tilt turn values
//======================================================================================================================
void backdash()
{
    if(aym<23)
    {
        if(axm<23)
        {
            buf = cycles;
        }
        if(buf>0)
        {
            buf--;
            if(axm<64)
            {
                gcc.xAxis = 128+ax*(axm<23);
            }
        }
    }
    else buf = 0;
}
//======================================================================================================================
// Ensures close to 0 values are reported as 0 on the sticks to fix dolphin calibration and allows user to switch to dolphin mode for backdash
//======================================================================================================================
void dolphinfix()
{
    if(axm<5&&aym<5)
    {
        gcc.xAxis  = 128; gcc.yAxis  = 128;
    }
    if(cxm<5&&cym<5)
    {
        gcc.cxAxis = 128; gcc.cyAxis = 128;
    }
    if(gcc.dright&&mode<2500)
    {
        dolphin = dolphin||(mode++>2000);
    }
    else mode = 0;

    cycles = 3 + (6*dolphin);
}
//======================================================================================================================
// This function will turn off all code is the LEFT on the dpad is held for 10 seconds
//======================================================================================================================
void nocode()
{
    if(gcc.dleft)
    {
        if(n == 0)
        {
            n = millis();
        }

        off = off||(millis()-n>2000);

    }
    else n = 0;
}
//======================================================================================================================
// Function to return angle in degrees when given x and y components
//======================================================================================================================
float ang(float xval, float yval)
{
    return atan(yval/xval)*57.2958;
}

//======================================================================================================================
// Function to return vector magnitude when given x and y components
//======================================================================================================================
float mag(char  xval, char  yval)
{
    return sqrt(sq(xval)+sq(yval));
}
//======================================================================================================================
// Set up initial values for the program to run
//======================================================================================================================
void setup()
{
    // Initial Values
    gcc.origin  = 0;
    gcc.errlatch= 0;
    gcc.high1   = 0;
    gcc.errstat = 0;

    // Calculates angle of SW gate base on user inputted data
    swang = ang(abs(sw_notch_x_value), abs(sw_notch_y_value));

    // Calculates angle of SE gate based on user inputted data
    seang = ang(abs(se_notch_x_value), abs(se_notch_y_value));
}
//======================================================================================================================
// Main loop for the program to run
//======================================================================================================================
void loop()
{
    // Read input from the controller and generate a report
    controller.read();
    gcc = controller.getReport();

    // Offsets from nuetral position of analog stick
    ax = gcc.xAxis -128; ay = gcc.yAxis -128;

    // Offsets from nuetral position of c stick
    cx = gcc.cxAxis-128; cy = gcc.cyAxis-128;

    // Magnitude of analog stick offsets
    axm = abs(ax); aym = abs(ay);

    // Magnitude of c stick offsets
    cxm = abs(cx); cym = abs(cy);

    // Determine if the user wants the code off
    if(!off)
    {
        // Implements all the fixes (remove this line to unmod the controller)
        convertinputs();
    }

    // Sends controller data to the console
    console.write(gcc);
}