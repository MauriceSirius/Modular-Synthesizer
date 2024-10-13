#include <SPI.h>
#include <MIDI.h>

// MCP4822 pin definitions
const int csPin = 10;  // Chip Select for MCP4822
const int gatePin = 2;  // Pin for GATE signal (D2)

// Constants for MIDI and DAC
const int lowestMIDINote = 36;  // C0 is MIDI note 36
const int highestMIDINote = 123;  // D#8 is MIDI note 123
const float opAmpGain = 1.77;  // Op-amp gain for 1V/octave scaling

// DAC Gain Mode (0 = 1X, 1 = 2X)
const bool gain = 1;  // Use 2X gain for DAC

// Define MIDI instance
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// Set the voltage (using the method from the provided code)
void setVoltage(int dacpin, bool channel, bool gain, unsigned int mV) {
  unsigned int command = channel ? 0x9000 : 0x1000;
  command |= gain ? 0x0000 : 0x2000;
  command |= (mV & 0x0FFF);

  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(dacpin, LOW);
  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);
  digitalWrite(dacpin, HIGH);
  SPI.endTransaction();
}

// Function to convert MIDI note to millivolts for the DAC
unsigned int midiToMV(int note) {
  // Constrain the MIDI note to the allowed range
  note = constrain(note, lowestMIDINote, highestMIDINote);

  // Calculate millivolts using 1V/octave scaling (47.069 mV per semitone)
  const float NOTE_SF = 47.069f;  // Scale factor: ~47.069 mV per semitone
  unsigned int mV = (unsigned int)((float)(note - lowestMIDINote) * NOTE_SF + 0.5);

  return mV;
}

// This function gets called when a Note On event is received
void handleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity > 0) {
    // Convert MIDI note to millivolts and send to DAC
    unsigned int mV = midiToMV(note);
    setVoltage(csPin, 0, gain, mV);

    // Set the GATE signal to HIGH (5V) to indicate a note is playing
    digitalWrite(gatePin, HIGH);
  }
}

// This function gets called when a Note Off event is received
void handleNoteOff(byte channel, byte note, byte velocity) {
  // Only reset the GATE signal, keep the DAC at its last voltage
  digitalWrite(gatePin, LOW);  // Turn off the GATE signal
}

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);
  while (!Serial);  // Wait for Serial to be ready

  // Initialize SPI for MCP4822 DAC
  SPI.begin();
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);  // Start with CS pin high (deselect DAC)

  // Initialize the GATE pin as output
  pinMode(gatePin, OUTPUT);
  digitalWrite(gatePin, LOW);  // Start with GATE pin LOW (0V)

  // Setup MIDI communication and callbacks
  MIDI.begin(MIDI_CHANNEL_OMNI);  // Listen to all MIDI channels
  MIDI.setHandleNoteOn(handleNoteOn);  // Set the callback for Note On
  MIDI.setHandleNoteOff(handleNoteOff);  // Set the callback for Note Off

  Serial.println("Listening for MIDI data...");
}

void loop() {
  // Listen for incoming MIDI messages
  MIDI.read();
}
