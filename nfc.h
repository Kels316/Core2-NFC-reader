#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <MFRC522_I2C.h>  // MFRC522-I2C library by arozcan

// RFID2 unit is on Grove Port A (I2C) at address 0x28
#define RFID2_I2C_ADDRESS 0x28
#define RFID2_RESET_PIN   -1    // not wired to a GPIO, use software reset

class NFCReader {
public:
    NFCReader();
    bool begin();
    
    // Returns UID as uppercase hex string (e.g. "A1B2C3D4") if a new tag
    // is present, or empty string if nothing is there.
    // Non-blocking — call every loop iteration.
    String readUID();

    // Returns the raw UID bytes from the last successful read
    const byte* lastUIDBytes() const { return _uid; }
    byte lastUIDLength() const { return _uidLength; }

private:
    MFRC522_I2C _mfrc522;
    byte _uid[10];
    byte _uidLength;
    String _uidToHex();
};
