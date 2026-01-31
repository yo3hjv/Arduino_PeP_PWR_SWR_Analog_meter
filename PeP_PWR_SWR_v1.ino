/*
================================================================================
  PWR/SWR Meter with Peak Envelope Power (PeP) Display
================================================================================

  PURPOSE:
  Arduino-based RF Power and SWR meter that reads Forward and Reflected voltages
  from a directional coupler, calculates power and SWR values, and displays them
  on a classic analog milliammeter.

  INNOVATION:
  Uses PWM output to drive an analog panel meter (milliammeter), providing a
  retro-style analog display for digital measurements. A rotary function switch
  allows the user to select which parameter to display:
    - AVG PWR:  Average RF power
    - PeP PWR:  Peak Envelope Power (maximum power captured)
    - SWR:      Current Standing Wave Ratio
    - PeP SWR:  Peak SWR (maximum SWR captured)
  
  Peak values (PeP PWR and PeP SWR) are retained until manually reset via a
  push button, enabling capture of transient maximum readings.

  TECHNICAL NOTES:
  - ADC samples Forward (Direct) and Reflected voltages with configurable interval
  - Multiple samples are averaged for noise reduction
  - PWM scaling: Power 0-130W FSD, SWR 1:1 to 10:1 FSD
  - All function inputs use internal pull-up resistors (active LOW)

================================================================================
  Version: 1.1
  Copyright: All rights reserved Adrian Florescu YO3HJV 2026
================================================================================
*/

//#define DEBUG_SERIAL  // Comment this line to disable Serial debug output

const byte pin_dir = A0;     // ADC pin for Direct voltage
const byte pin_rev = A2;     // ADC pin for Reverse voltage
const byte PeakResetPin = 7; // Pin for PeakReset button
const byte maPin = 9;        // PWM output pin

const byte pinF1 = 3;       // Function pin: AVG PWR (average power)
const byte pinF2 = 4;       // Function pin: PeP PWR (peak envelope power)
const byte pinF3 = 5;       // Function pin: SWR (current standing wave ratio)
const byte pinF6 = 6;       // Function pin: PeP SWR (peak SWR / SWR Max)

volatile float dir_v_averaged = 0;  // Averaged Direct voltage
volatile float rev_v_averaged = 0;  // Averaged Reverse voltage
volatile float peak_v_averaged = 0; // Peak voltage
volatile float swr_v = 0.0;         // Global variable to store the SWR value
volatile float swrMax = 1.0;        // Global variable to store SWR Max (peak SWR)
volatile float PWR = 0;             // global variable to store PWR
volatile float pPWR=0;              // global variable to store Peak Power
float ppPWR = 0;

unsigned long lastButtonPressTime = 0;  // To store the last button press time
const unsigned long debounceDelay = 200; // Debounce delay to avoid multiple triggers

unsigned long lastPrintTime = 0;  // Stores the last print time
const unsigned long printInterval = 500;  // Print every 500ms

float cor_adc = 0.858;                 // correction factor for ADC reference
float pwr_factor = 8;             // to accurate calibrate PWR indicator
float swr_factor = 0;             // to accurate calibrate SWR indicator

const byte num_samples = 3;
const unsigned long adcSamplingInterval = 5;  // ADC sampling interval in ms (0 = no delay, continuous)
unsigned long lastAdcSampleTime = 0;          // Last ADC sample timestamp


void calculatePWR() {   // calculez puterea avg si puterea peak
        PWR = pwr_factor * dir_v_averaged;
         if (PWR > pPWR) { // If the current PWR exceeds peak_PWR, update peak_PWR
        pPWR = PWR; // Store the highest power value
        ppPWR = pPWR ;  // Corectie pentru overshoot
    }
}

void send_to_mA() {                            
                            float selectedValue = 0;  
                            int pwmValue = 0;  // Initialize pwmValue before using it

                            if (digitalRead(pinF1) == LOW) {
                                selectedValue = PWR;  // Send Power meter value
                            } 
                            else if (digitalRead(pinF2) == LOW) {
                                
                                selectedValue = pPWR;  // Send Peak Power
                               
                            } 
                            else if (digitalRead(pinF3) == LOW) {
                                selectedValue = swr_v;  // Send SWR
                            } 
                            else if (digitalRead(pinF6) == LOW) {
                                selectedValue = swrMax;  // Send SWR Max
                            } 
                            else {
                                selectedValue = 0;  // Default case (no pin grounded)
                            }

                            // Convert selected value to PWM (0-255)
                            if (selectedValue == swr_v || selectedValue == swrMax) {
                                pwmValue = ((selectedValue - 1) / 9.0) * 255;

                                //pwmValue = (selectedValue / 10.0) * 255;  // SWR has a different scaling
                            } else {
                                pwmValue = (selectedValue / 130.0) * 255;  // Power scaling
                            }

                            pwmValue = constrain(pwmValue, 0, 255);  // Ensure PWM is within valid range

                            // Output PWM signal
                            analogWrite(maPin, pwmValue);
}


// Function to calculate SWR
void calculateSWR() {
     //     if (dir_v_averaged == 0) {
     //         swr_v = 0.0;  // Handle case when dir_v_averaged is zero/ Cand nu exista semnal la borna, indica 0. SWR minim este 1 iar cand apare inseamna ca avem RF la borna

     //         return;
     //             }
          if (rev_v_averaged < 0.05) {
              swr_v = 1;  // Handle case when rev_v_averaged is noise or zero
              return;
                  }
          
     // Ensure dir_v_averaged > rev_v_averaged before calculation
    if (rev_v_averaged >= dir_v_averaged) {
        swr_v = INFINITY;  // Perfect reflection scenario (open/short)
        return;
    }

    // Calculate SWR using the proper formula
    float ratio = rev_v_averaged / dir_v_averaged;
    swr_v =  (1 + ratio) / (1 - ratio);
    
    // Track SWR Max (similar to PeP)
    if (swr_v > swrMax && swr_v != INFINITY) {
        swrMax = swr_v;
    }
}


void readAndAverage() {
                    // Check if enough time has passed since last ADC sample
                    if (adcSamplingInterval > 0) {
                        unsigned long currentTime = millis();
                        if (currentTime - lastAdcSampleTime < adcSamplingInterval) {
                            return;  // Skip sampling if interval not reached
                        }
                        lastAdcSampleTime = currentTime;
                    }
                    
                    int dir_sum = 0, rev_sum = 0;

                    for (int i = 0; i < num_samples; i++) {
                        dir_sum += analogRead(pin_dir);
                        rev_sum += analogRead(pin_rev);
                    }

                    // Compute the averaged ADC values and convert to voltage
                    dir_v_averaged = cor_adc* (dir_sum / float(num_samples)) * (5 / 1023.0);
                    rev_v_averaged = cor_adc* (rev_sum / float(num_samples)) * (5 / 1023.0);
}


void handlePeakReset() {
          if (digitalRead(PeakResetPin) == LOW) {
              unsigned long currentTime = millis();

              if (currentTime - lastButtonPressTime > debounceDelay) {
                  pPWR  = 0;  // Reset the peak voltage
                  swrMax = 1.0;  // Reset SWR Max to minimum value
                  lastButtonPressTime = currentTime;  // Update the last press time
              }
          }
}



void setup() {
          #ifdef DEBUG_SERIAL
          Serial.begin(115200);
          #endif
          pinMode(PeakResetPin, INPUT_PULLUP);  // Set the PeakReset button pin as input with pull-up resistor

          pinMode(pinF1, INPUT_PULLUP);
          pinMode(pinF2, INPUT_PULLUP);
          pinMode(pinF3, INPUT_PULLUP);
          pinMode(pinF6, INPUT_PULLUP);

}

void loop() {
          readAndAverage();
          handlePeakReset();  // Check if the button is pressed to reset the peak voltage
          calculatePWR();
          calculateSWR();  // Calculate and update SWR
          send_to_mA();  // Send PWM signal proportional to peak_v_averaged
          #ifdef DEBUG_SERIAL
          serialDebug();
          #endif
}


#ifdef DEBUG_SERIAL
void serialDebug() {
          if (millis() - lastPrintTime >= printInterval) {
              lastPrintTime = millis();
              Serial.print("  DirV: ");
              Serial.print(dir_v_averaged, 3);
              Serial.print("   RevV: ");
              Serial.print(rev_v_averaged, 3);
              Serial.print("   SWR: ");
              Serial.print(swr_v, 2);
              Serial.print("   swrMax: ");
              Serial.print(swrMax, 2);
              Serial.print("   PWR: ");
              Serial.print(PWR, 1);
              Serial.print("   pPWR: ");
              Serial.println(pPWR, 1);
          }
}
#endif
