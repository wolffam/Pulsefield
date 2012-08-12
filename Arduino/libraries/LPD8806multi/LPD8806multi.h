#if (ARDUINO >= 100)
 #include <Arduino.h>
#else
 #include <WProgram.h>
 #include <pins_arduino.h>
#endif

class LPD8806multi {
 public:
  LPD8806multi();
  void init(void);
  void show(void);
  void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
  void setPixelColor(uint16_t n, uint32_t c);
  uint16_t numPixels(void);
  uint32_t  Color(byte, byte, byte);
  uint32_t getPixelColor(uint16_t n);

  // These primarily exist for debugging and will likely come out later:
  uint8_t pause;  // Delay (in milliseconds) after latch
 private:
  uint16_t numLEDs; // Number of RGB LEDs in strip
  uint8_t *pixels;  // Holds LED color values (3 bytes each)
  void alloc(uint16_t n);
  void writeLatch();
  boolean begun;       // If 'true', begin() method was previously invoked
};
