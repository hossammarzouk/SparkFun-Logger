
*/
const byte power_led  = 29;
const byte status_led = 19;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(power_led, OUTPUT);
  pinMode(status_led, OUTPUT);

}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(power_led, HIGH);     // turn the LED on (HIGH is the voltage level)
  digitalWrite(status_led, LOW);     // turn the LED off by making the voltage LOW
  delay(500);                       // wait for a second
  digitalWrite(status_led, HIGH);    // turn the LED off by making the voltage LOW
  digitalWrite(power_led, LOW);      // turn the LED on (HIGH is the voltage level)
  delay(500);
  }
