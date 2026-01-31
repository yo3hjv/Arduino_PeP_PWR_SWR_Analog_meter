/*   Versiunea Dezvoltare_1

ADC masoara in fereastra de esantionare tensiunea DIR si REV apoi scoate media din mai multe esantioane.
Permanent, cele doua tensiuni sunt incarcate in valori globale.
Tot permanent, valoarea maxima a tensiunii DIR este actualizata in peak_v_averaged, variabile globala.
Valoarea acestei variabile se reseteaza la 0 cu butonul PeakResetPin.
ATENTIE!
Variabilele actualizate rapid sunt TENSIUNI (volti) si nu PUTERI!

In varianta 'basic', valorile variabilelor sunt actualizate la cca 600 uSec.
in varianta asta, la cca 4-6 milisec.
*/

const byte pin_dir = A0;     // ADC pin for Direct voltage
const byte pin_rev = A2;     // ADC pin for Reverse voltage
const byte PeakResetPin = 7; // Pin for PeakReset button
const byte maPin = 9;        // PWM output pin

const byte pinF1 = 3;       // Function pins for rotary switch
const byte pinF2 = 4;
const byte pinF3 = 5;

volatile float dir_v_averaged = 0;  // Averaged Direct voltage
volatile float rev_v_averaged = 0;  // Averaged Reverse voltage
volatile float peak_v_averaged = 0; // Peak voltage
volatile float swr_v = 0.0;         // Global variable to store the SWR value
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
                            else {
                                selectedValue = 0;  // Default case (no pin grounded)
                            }

                            // Convert selected value to PWM (0-255)
                            if (selectedValue == swr_v) {
                                pwmValue = ((selectedValue - 1) / 9.0) * 255;

                                //pwmValue = (selectedValue / 10.0) * 255;  // SWR has a different scaling
                            } else {
                                pwmValue = (selectedValue / 130.0) * 255;  // Power scaling
                            }

                            pwmValue = constrain(pwmValue, 0, 255);  // Ensure PWM is within valid range

                            // Output PWM signal
                            analogWrite(maPin, pwmValue);

                            // Debugging output
                            Serial.print("  DirV: ");
                            Serial.print(dir_v_averaged,3);
                              Serial.print("   RevV: ");
                               Serial.print(rev_v_averaged,3);
                            Serial.print("   Selected Value: ");
                            Serial.print(selectedValue);
                            Serial.print("   PWM Output: ");
                            Serial.println(pwmValue);
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
}


void readAndAverage() {
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
                  lastButtonPressTime = currentTime;  // Update the last press time
              }
          }
}



void setup() {
          Serial.begin(115200);
          pinMode(PeakResetPin, INPUT_PULLUP);  // Set the PeakReset button pin as input with pull-up resistor

          pinMode(pinF1, INPUT_PULLUP);
          pinMode(pinF2, INPUT_PULLUP);
          pinMode(pinF3, INPUT_PULLUP);

}

void loop() {
          readAndAverage();
          handlePeakReset();  // Check if the button is pressed to reset the peak voltage
          calculatePWR();
          calculateSWR();  // Calculate and update SWR
          send_to_mA();  // Send PWM signal proportional to peak_v_averaged

}
