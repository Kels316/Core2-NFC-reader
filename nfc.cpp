#include "nfc.h"

NFCReader::NFCReader()
    : _mfrc522(RFID2_I2C_ADDRESS, RFID2_RESET_PIN)
    , _uidLength(0)
{
    memset(_uid, 0, sizeof(_uid));
}

bool NFCReader::begin() {
    Wire.begin();
    _mfrc522.PCD_Init();
    delay(50);

    // Verify the reader is responding — same pattern as Pi RC522 init check
    byte version = _mfrc522.PCD_ReadRegister(MFRC522_I2C::VersionReg);
    if (version == 0x00 || version == 0xFF) {
        Serial.println("[NFC] RFID2 not responding (version=0x" + String(version, HEX) + ")");
        return false;
    }
    Serial.println("[NFC] RFID2 ready, version=0x" + String(version, HEX));
    return true;
}

String NFCReader::readUID() {
    // Non-blocking poll — returns "" if no tag present
    if (!_mfrc522.PICC_IsNewCardPresent()) {
        return "";
    }
    if (!_mfrc522.PICC_ReadCardSerial()) {
        return "";
    }

    // Copy UID out
    _uidLength = _mfrc522.uid.size;
    memcpy(_uid, _mfrc522.uid.uidByte, _uidLength);

    // Halt the card so it doesn't keep re-triggering until removed and re-presented
    _mfrc522.PICC_HaltA();

    return _uidToHex();
}

String NFCReader::_uidToHex() {
    String result = "";
    for (byte i = 0; i < _uidLength; i++) {
        if (_uid[i] < 0x10) result += "0";
        result += String(_uid[i], HEX);
    }
    result.toUpperCase();
    return result;
}
