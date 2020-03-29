//#include <SD.h>

#include "SdFat.h"
SdFat SD;

#include <SPI.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

const char *filename = "/config.cfg";

const int NUM_LOOPS = 6;
const int NUM_SWITCHES = 2;
const int NUM_PRESETS = 18;

const int MOMENTARY_MS = 150;

const int CS_PIN = 10;

const byte LOOP_PINS[NUM_LOOPS] = {2, 3, 4, 5, 6, 7};
const byte SWITCH_PINS[NUM_SWITCHES] = {8, 9};

const byte ACTIVATE_LOOP_MIN = 111;
const byte ACTIVATE_LOOP_MAX = ACTIVATE_LOOP_MIN + NUM_LOOPS - 1;
const byte BYPASS_LOOP_MIN = 101;
const byte BYPASS_LOOP_MAX = BYPASS_LOOP_MIN + NUM_LOOPS - 1;
const byte ACTIVATE_SWITCH_MIN = ACTIVATE_LOOP_MIN + NUM_LOOPS;
const byte ACTIVATE_SWITCH_MAX = ACTIVATE_SWITCH_MIN + NUM_SWITCHES - 1;
const byte BYPASS_SWITCH_MIN = BYPASS_LOOP_MIN + NUM_LOOPS;
const byte BYPASS_SWITCH_MAX = BYPASS_SWITCH_MIN + NUM_SWITCHES - 1;

struct Preset
{
    byte loops[NUM_LOOPS];
    byte switches[NUM_SWITCHES];
};

struct Config
{
    byte MIDI_CHANNEL;
    byte switchTypes[NUM_SWITCHES];
};

Preset presets[NUM_PRESETS];
Config config;

bool configLoaded = false;

void setup()
{
    if (!SD.begin(CS_PIN))
    {
        loadConfigFallback();
        configLoaded = false;
    }
    else
    {
        loadConfiguration(filename);
        configLoaded = true;
    }


    // Set loop & switch pins
    for (byte i = 0; i < NUM_LOOPS; i++)
        pinMode(LOOP_PINS[i], OUTPUT);
    for (byte i = 0; i < NUM_SWITCHES; i++)
        pinMode(SWITCH_PINS[i], OUTPUT);

    MIDI.begin(config.MIDI_CHANNEL);
    MIDI.setHandleProgramChange(handleProgramChange);

    // Initialize serial port
    // Disable when activating MIDI
    // Serial.begin(9600);
    // while (!Serial)
    //     continue;

    // printConfig();
}

void loop()
{
    MIDI.read();
}

void handleProgramChange(byte channel, byte number)
{
    if (channel == config.MIDI_CHANNEL)
    {
        if (number == 0)
            resetAll();
        else if ((number <= NUM_PRESETS) && configLoaded)
            loadPreset(number - 1);
        else if (
            (number >= ACTIVATE_LOOP_MIN && number <= ACTIVATE_LOOP_MAX) ||
            (number >= BYPASS_LOOP_MIN && number <= BYPASS_LOOP_MAX))
        {
            byte loopNum = (number % 10) - 1;
            byte loopState = number / 10 % 10;
            digitalWrite(LOOP_PINS[loopNum], loopState);
        }
        else if (
            (number >= ACTIVATE_SWITCH_MIN && number <= ACTIVATE_SWITCH_MAX) ||
            (number >= BYPASS_SWITCH_MIN && number <= BYPASS_SWITCH_MAX))
        {
            byte switchNum = (number % 10) - 1 - NUM_LOOPS;
            byte switchState = number / 10 % 10;
            setSwitch(SWITCH_PINS[switchNum], config.switchTypes[switchNum], switchState);
        }
    }
}

void loadPreset(byte presetNum)
{
    for (byte i = 0; i < NUM_LOOPS; i++)
        digitalWrite(LOOP_PINS[i], presets[presetNum].loops[i]);

    for (byte i = 0; i < NUM_SWITCHES; i++)
        setSwitch(SWITCH_PINS[i], config.switchTypes[i], presets[presetNum].switches[i]);
}

void setSwitch(byte switchPin, byte switchType, byte switchVal)
{

    if (switchType == 1)
    {
        digitalWrite(switchPin, switchVal);
    }
    else
    {
        if (switchVal == 1)
        {
            digitalWrite(switchPin, switchVal);
            delay(MOMENTARY_MS);
            digitalWrite(switchPin, !switchVal);
        }
    }
}

void resetAll()
{
    for (byte i = 0; i < NUM_LOOPS; i++)
        digitalWrite(LOOP_PINS[i], 0);

    for (byte i = 0; i < NUM_SWITCHES; i++)
        setSwitch(SWITCH_PINS[i], config.switchTypes[i], 0);
}

// Loads the configuration from a file
void loadConfiguration(char *filename)
{
    File file = SD.open(filename);
    if (!file)
    {
        Serial.println(F("Failed to read file"));
        return;
    }

    // Read MIDI channel
    String midiChannelStr = "";
    midiChannelStr += (char)file.read() - 48;
    midiChannelStr += (char)file.read() - 48;

    config.MIDI_CHANNEL = midiChannelStr.toInt();
    file.read();
    file.read();

    // Read relay switch types
    // 1: Latching
    // 0: Momentary
    for (byte i = 0; i < NUM_SWITCHES; i++)
        config.switchTypes[i] = (char)file.read() - 48;
    file.read();
    file.read();

    // Read presets
    for (byte i = 0; i < NUM_PRESETS; i++)
    {
        if (file.available())
        {
            // Read loop status
            for (byte j = 0; j < NUM_LOOPS; j++)
                presets[i].loops[j] = (char)file.read() - 48;
            file.read();

            // Read switch status
            for (byte j = 0; j < NUM_SWITCHES; j++)
                presets[i].switches[j] = (char)file.read() - 48;
            file.read();
            file.read();
        }
    }

    file.close();
}

void loadConfigFallback()
{
    // If fail to read to SD card, at least have minimal config

    config.MIDI_CHANNEL = 16;

    // Set all switches to momentary
    for (byte i = 0; i < NUM_SWITCHES; i++)
        config.switchTypes[i] = 0;

    // No presets available

}

void printConfig()
{
    Serial.print("MIDI Channel#: ");
    Serial.println(config.MIDI_CHANNEL);

    Serial.println("Switch Types:");
    for (byte i = 0; i < NUM_SWITCHES; i++)
    {
        Serial.print("\tSwitch #");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(config.switchTypes[i]);
    }

    Serial.println("Programs:");
    for (byte i = 0; i < NUM_PRESETS; i++)
    {
        Serial.print("\tProgram #");
        Serial.print(i + 1);
        Serial.print(" | ");

        Serial.print("Loops: ");
        for (byte j = 0; j < NUM_LOOPS; j++)
            Serial.print(presets[i].loops[j]);
        Serial.print(" | ");

        Serial.print("Switches: ");
        for (byte j = 0; j < NUM_SWITCHES; j++)
            Serial.print(presets[i].switches[j]);
        Serial.println("");
    }
}
