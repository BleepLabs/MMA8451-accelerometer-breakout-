/*
  Using the Bleep labs MMA8451 accelerometer
  The board we're using is similar to this https://www.adafruit.com/product/2019 but I made it a long time ago before it was available from adafruit but never put it in a product
  This code uses the adafruit library

  This example sends USB MIDI usb midi https://www.pjrc.com/teensy/td_midi.html
  Set USB mode using "Tools>USB type>Serial+MIDI"
*/

#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

#include <Wire.h>

//name the snesor and get the library working
Adafruit_MMA8451 accelo = Adafruit_MMA8451();

unsigned long current_time, prev[4];
int x_read, y_read, z_read;
int orientation, prev_orientation;
int  smoothed_x;
int  smoothed_y;
int  smoothed_z;
int z_out, prev_z_out;

int note_latch[4];
unsigned long note_latch_time[4];

void setup(void) {
  delay(100);

  while (!Serial) { //while no serial communication...
    //...don't do anything until you open the serial monitor
  }

  //if the accelo doesn't begin...
  while (!accelo.begin(0x1C)) { //necessary for this board. adafruit board has different address
    Serial.println("Couldnt start. Check connections");
    delay(500);
    accelo.begin(0x1C); //keep trying
  }
  Serial.println("MMA8451 found!");

  //can be "MMA8451_RANGE_2_G", "MMA8451_RANGE_4_G" or "MMA8451_RANGE_8_G"
  // the bigger the number,the less sensitive
  accelo.setRange(MMA8451_RANGE_2_G);

}

void loop() {
  current_time = millis();

  // Read the raw data. fills the x y z with bipolar 14 bit values, -8191 to 8191
  accelo.read();

  //we don't need all that resolution so lets divide it down to -1000 to 1000 to make it easier to understand
  // since 8191 is our biggest number just move the decimal in that and divide by it
  x_read = accelo.x / 8.191; //these can only be done after accelo.read()
  y_read = accelo.y / 8.191;
  z_read = accelo.z / 8.191;


  //smooth(select, number of readings, input);
  // select needs to be different for each variable you want to smooth
  // number of readings can be anything from 7-99 and should be odd for best results.
  // The larger the number the more smooth but the less responsive the result will be

  //smooth(select, number of readings, input)
  // select should be different for every different input
  // the more redaing the smoother. You could also reduce the sample rate by puting this in a timing "if"
  smoothed_x = smooth(0, 41, x_read);
  smoothed_y = smooth(1, 41, y_read);
  smoothed_z = smooth(2, 41, z_read);

  if (current_time - prev[1] > 5) { //this shouldnt happen too fast so it's in a timing "if"
    prev[1] = current_time;
    prev_z_out = z_out;
    z_out = map(smoothed_z, -1000, 1000, 0, 127); //midi is 0-127

    if (prev_z_out != z_out) {
      usbMIDI.sendControlChange(20, z_out, 1);
    }
  }


  if (current_time - prev[0] > 40) {
    prev[0] = current_time;

    int print_smooth = 1;

    if (print_smooth == 0) {
      Serial.print(x_read);
      Serial.print(" ");
      Serial.print(y_read);
      Serial.print(" ");
      Serial.print(z_read);
      Serial.println();
    }

    if (print_smooth == 1) {
      Serial.print(smoothed_x);
      Serial.print(" ");
      Serial.print(smoothed_y);
      Serial.print(" ");
      Serial.print(smoothed_z);
      Serial.println();
    }


    //this device works just like the accelo in your phone that rotates the screen.
    // this function gets a value from 0-8 which then we print as a phone based orientation.
    prev_orientation = orientation;
    orientation = accelo.getOrientation();

    //"switch case" is similar to "if" but you give it a pile of possible outcomes and it picks just one
    // then it "breaks", leaving the switch and going on to the next thing
    // https://www.arduino.cc/reference/en/language/structure/control-structure/switchcase/
    switch (orientation) {
      //these are defines in the adafruit library. "#define MMA8451_PL_PUF 0" https://github.com/adafruit/Adafruit_MMA8451_Library/blob/c7f64f04f00a16b6c786677db4fc75eec65fabdd/Adafruit_MMA8451.h#L45
      // so this is just the same as saying:
      // case 0:
      case MMA8451_PL_PUF:
        Serial.println("Portrait Up Front");
        if (prev_orientation != orientation) {
          usbMIDI.sendNoteOn(40, 127, 1);
          note_latch[0] = 1;
          note_latch_time[0] = current_time;
        }
        //you could put whatever you want here. maybe it could trigger different sounds
        break;
      case MMA8451_PL_PUB:
        Serial.println("Portrait Up Back");
        break;
      case MMA8451_PL_PDF:
        Serial.println("Portrait Down Front");
        break;
      case MMA8451_PL_PDB:
        Serial.println("Portrait Down Back");
        break;
      case MMA8451_PL_LRF:
        Serial.println("Landscape Right Front");
        break;
      case MMA8451_PL_LRB:
        Serial.println("Landscape Right Back");
        break;
      case MMA8451_PL_LLF:
        Serial.println("Landscape Left Front");
        break;
      case MMA8451_PL_LLB:
        Serial.println("Landscape Left Back");
        break;
    }
    Serial.println(); // print a return to space it out

  }

  if (note_latch[0] == 1) {
    if (current_time - note_latch_time[0] > 1000) {
      usbMIDI.sendNoteOff(40, 0, 1);
      note_latch[0] = 0;
    }
  }

} // end of loop


// Smooth and scale functions

//based on https://playground.arduino.cc/Main/DigitalSmooth/
// This function continuously samples an input and puts it in an array that is "samples" in length.
// This array has a new "raw_in" value added to it each time "smooth" is called and an old value is removed
// It throws out the top and bottom 15% of readings and averages the rest

#define maxarrays 8 //max number of different variables to smooth
#define maxsamples 99 //max number of points to sample and 
//reduce these numbers to save RAM

unsigned int smoothArray[maxarrays][maxsamples];

// sel should be a unique number for each occurrence
// samples should be an odd number greater that 7. It's the length of the array. The larger the more smooth but less responsive
// raw_in is the input. positive numbers in and out only.

unsigned int smooth(byte sel, unsigned int samples, unsigned int raw_in) {
  int j, k, temp, top, bottom;
  long total;
  static int i[maxarrays];
  static int sorted[maxarrays][maxsamples];
  boolean done;

  i[sel] = (i[sel] + 1) % samples;    // increment counter and roll over if necessary. -  % (modulo operator) rolls over variable
  smoothArray[sel][i[sel]] = raw_in;                 // input new data into the oldest slot

  for (j = 0; j < samples; j++) { // transfer data array into anther array for sorting and averaging
    sorted[sel][j] = smoothArray[sel][j];
  }

  done = 0;                // flag to know when we're done sorting
  while (done != 1) {      // simple swap sort, sorts numbers from lowest to highest
    done = 1;
    for (j = 0; j < (samples - 1); j++) {
      if (sorted[sel][j] > sorted[sel][j + 1]) {    // numbers are out of order - swap
        temp = sorted[sel][j + 1];
        sorted[sel] [j + 1] =  sorted[sel][j] ;
        sorted[sel] [j] = temp;
        done = 0;
      }
    }
  }

  // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
  bottom = max(((samples * 15)  / 100), 1);
  top = min((((samples * 85) / 100) + 1  ), (samples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
  k = 0;
  total = 0;
  for ( j = bottom; j < top; j++) {
    total += sorted[sel][j];  // total remaining indices
    k++;
  }
  return total / k;    // divide by number of samples
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//floating point scaling
// works like map but allows for float results and applying a log or expo curve
// If curve is 0, it returns a linear scaling
// less than 0 and it's logarithmic
// greater than 0 and it's exponential

// based on
///https://playground.arduino.cc/Main/Fscale/
// there are more efficient ways of doing this but it works just fine for getting pots to other ranges.

float fscale(float inputValue,  float originalMin, float originalMax, float newBegin, float
             newEnd, float curve) {

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  if (curve > 1) curve = 1;
  if (curve < -1) curve = -1;

  float curve_amount = 1.0; // increase this number to get steeper curves
  curve = (curve * curve_amount * -1.0) ; // - invert and scale - this seems more intuitive - positive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into logarithmic exponent for other pow function


  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero reference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin) {
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0) {
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;
}
