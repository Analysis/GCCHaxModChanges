// Record southwest and southeast notch values
#define sw_notch_x_value -.7000
#define sw_notch_y_value -.7000
#define se_notch_x_value  .7000
#define se_notch_y_value -.7000

// To switch to dolphin mode hold dpad right for 5 seconds
// To return to a stock controller hold dpad left for 10 seconds

//======================================================================================================================
//    Install Instructions:
//
//    1. connect Arduino to computer and make sure proper board [Arduino Nano], processor [ATmega328], and COM
//       port [highest number] are selected under Tools.
//
//    2. hit the upload button (the arrow) and wait for it to say "Done Uploading" at the bottom before disconnecting
//       the Arduino.
//
//    3. connect 5V and Gnd from GCC header to 5V and Gnd pins on Arduino (do not sever either wire)
//
//    4. sever data wire between GCC cable and header on controller
//
//    5. connect D2 on Arduino to data header on controller, and D3 to data wire from cable
//
//    6. ensure no wires are caught anywhere and close everything up
//
//    Note: if there is any trouble with steps 1 or 2, get the CH340 driver for your operating system from google
//======================================================================================================================

#include <Arduino.h>
#include "Nintendo.h"

// Sets D2 on Arduino to read data from controller
CGamecubeController controller(2);

// Sets D3 on Arduino to write data to console
CGamecubeConsole console(3);

// Structural values for controller state
Gamecube_Report_t gamecubeReport;
bool shieldOn, tilt, dolphin = 0, turnCodeOff = 0;
byte axm, aym, cxm, cym, cycles;
char ax, ay, cx, cy, buf;
float swAngle, seAngle;
word mode, toggle;
unsigned long n;

//======================================================================================================================
// Reduce the dead zone in cardinal directions and give the steepest/shallowest angles when on or near the gate
//======================================================================================================================
void perfectAngles()
{
    if(axm>75)
    {
        gamecubeReport.xAxis = (ax>0)?204:52;
        if(aym<23)
        {
            gamecubeReport.yAxis = (ay>0)?151:105;
        }
    }
    if(aym>75)
    {
        gamecubeReport.yAxis = (ay>0)?204:52;
        if(axm<23)
        {
            gamecubeReport.xAxis = (ax>0)?151:105;
        }
    }
}
//======================================================================================================================
// Snaps sufficiently high cardinal inputs to vectors of 1.0 magnitude of analog stick and c stick
//======================================================================================================================
void maximizeVectors()
{
    if(axm>75&&aym< 9)
    {
        gamecubeReport.xAxis  = (ax>0)?255:1;
        gamecubeReport.yAxis  = 128;
    }
    if(aym>75&&axm< 9)
    {
        gamecubeReport.yAxis  = (ay>0)?255:1;
        gamecubeReport.xAxis  = 128;
    }
    if(cxm>75&&cym<23)
    {
        gamecubeReport.cxAxis = (cx>0)?255:1;
        gamecubeReport.cyAxis = 128;
    }
    if(cym>75&&cxm<23)
    {
        gamecubeReport.cyAxis = (cy>0)?255:1;
        gamecubeReport.cxAxis = 128;
    }
}
//======================================================================================================================
// Function to return angle in degrees when given x and y components
//======================================================================================================================
float angle(float xValue, float yValue)
{
    return atan(yValue/xValue)*57.2958;
}

//======================================================================================================================
// Function to return vector magnitude when given x and y components
//======================================================================================================================
float magnitude(char xValue, char yValue)
{
    return sqrt(sq(xValue)+sq(yValue));
}
//======================================================================================================================
//  Allows shield drops down and gives a 6 degree range of shield dropping centered on SW and SE gates
//======================================================================================================================
void buffShieldDrops()
{
    shieldOn = (gamecubeReport.l || gamecubeReport.r || gamecubeReport.left>74 || gamecubeReport.right>74 ||gamecubeReport.z);
    if(shieldOn)
    {
        if(ay<0&& magnitude(ax, ay)>75)
        {

            if(ax<0&&abs(angle(axm, aym)-swAngle)<4)
            {
                gamecubeReport.yAxis = 73; gamecubeReport.xAxis =  73;
            }

            if(ax>0&&abs(angle(axm, aym)-seAngle)<4)
            {
                gamecubeReport.yAxis = 73; gamecubeReport.xAxis = 183;
            }
        }
        else if(abs(ay+39)<17&&axm<23) gamecubeReport.yAxis = 73;
    }
}
//======================================================================================================================
// Fixes dashback by imposing a 1 frame buffer upon tilt turn values
//======================================================================================================================
void fixDashBack()
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
                gamecubeReport.xAxis = 128+ax*(axm<23);
            }
        }
    }
    else buf = 0;
}
//======================================================================================================================
// Ensures close to 0 values are reported as 0 on the sticks to fix dolphin calibration and allows user to switch to
// dolphin mode for backdash
//======================================================================================================================
void dolphinFix()
{
    if(axm<5&&aym<5)
    {
        gamecubeReport.xAxis  = 128; gamecubeReport.yAxis  = 128;
    }
    if(cxm<5&&cym<5)
    {
        gamecubeReport.cxAxis = 128; gamecubeReport.cyAxis = 128;
    }
    if(gamecubeReport.dright && mode<2500)
    {
        dolphin = dolphin||(mode++>2000);
    }
    else mode = 0;

    cycles = 3 + (6*dolphin);
}
//======================================================================================================================
// This function will turn off all code is the LEFT on the dpad is held for 10 seconds
//======================================================================================================================
void determineCodeOff()
{
    // If the user holds DPAD LEFT, count for 10 seconds.
    if(gamecubeReport.dleft)
    {
        if(n == 0)
        {
            n = millis();
        }

        turnCodeOff = turnCodeOff||(millis()-n>2000);

    }
    else n = 0;
}
//======================================================================================================================
// Master method to run convert the inputs with their respective mods.
//======================================================================================================================
void convertInputs()
{
    // Reduces dead-zone of cardinals and gives steepest/shallowest angles when on or near the gate
    perfectAngles();

    // Snaps sufficiently high cardinal inputs to vectors of 1.0 magnitude of analog stick and c stick
    maximizeVectors();

    // Allows shield drops down and gives a 6 degree range of shield dropping centered on SW and SE gates
    buffShieldDrops();

    // Fixes dashback by imposing a 1 frame buffer upon tilt turn values
    fixDashBack();

    // Ensures close to 0 values are reported as 0 on the sticks to fix dolphin calibration and allows user to switch to dolphin mode for backdash
    dolphinFix();

    // Function to disable all code if DPAD LEFT is held for 10 seconds
    determineCodeOff();
}
//======================================================================================================================
// Set up initial values for the program to run
//======================================================================================================================
void setup()
{
    // Initial Values
    gamecubeReport.origin  = 0;
    gamecubeReport.errlatch= 0;
    gamecubeReport.high1   = 0;
    gamecubeReport.errstat = 0;

    // Calculates angle of SW gate base on user inputted data
    swAngle = angle(abs(sw_notch_x_value), abs(sw_notch_y_value));

    // Calculates angle of SE gate based on user inputted data
    seAngle = angle(abs(se_notch_x_value), abs(se_notch_y_value));
}
//======================================================================================================================
// Main loop for the program to run
//======================================================================================================================
void loop()
{
    // Read input from the controller and generate a report
    controller.read();
    gamecubeReport = controller.getReport();

    // Offsets from neutral position of analog stick
    ax = gamecubeReport.xAxis -128;
    ay = gamecubeReport.yAxis -128;

    // Offsets from neutral position of c stick
    cx = gamecubeReport.cxAxis-128;
    cy = gamecubeReport.cyAxis-128;

    // Magnitude of analog stick offsets
    axm = abs(ax);
    aym = abs(ay);

    // Magnitude of c stick offsets
    cxm = abs(cx);
    cym = abs(cy);

    // Determine if the user wants the code off
    if(!turnCodeOff)
    {
        // Implements all the fixes (remove this line to un-mod the controller completely)
        convertInputs();
    }

    // Sends controller data to the console
    console.write(gamecubeReport);
}