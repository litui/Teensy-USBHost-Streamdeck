# Teensy USBHost Streamdeck

This is an add-on library for supporting the Stream Deck MkII (and eventually more) on the Teensy 4.x with USBHost_t36 library.

Right now it's just barebones - set brightness, load pre-prepared images.

## Usage:

There are two hook points you can add your own callbacks for:
* Single key press/release hook - `void attachSinglePress(void (*f)(StreamdeckController *sdc, const uint16_t keyIndex, const uint8_t newValue, const uint8_t oldValue))`
* Press/release hook showing all key states at once - `void attachAnyChange(void (*f)(StreamdeckController *sdc, const uint8_t *newStates, const uint8_t *oldStates))`

There are a handful of useful functions you can call from your script when the controller is attached/active:
* `void setBrightness(float percent)` - sets brightness; percent values are floats between 0 and 1
* `void setKeyImage(const uint16_t keyIndex, const uint8_t *image, const uint16_t length)` - lets you set a jpeg-formatted image to a key
* `void setKeyBlank(const uint16_t keyIndex)` - sets a key to black
* `uint16_t getNumKeys()` - retrieves the number of keys/states available
* `void reset()` - issues a reset! Don't do this for now; it irrevocably resets the pipes
* `void flushImageReports()` - clears the pending queue and sends an empty outbound report to reset counters on the Streamdeck
* `void blankAllKeys();` - shortcut to set all keys to blank (black)

## Todo:

* Add support for other Stream Decks, ideally mirroring [python-elgato-streamdeck](https://github.com/abcminiuser/python-elgato-streamdeck/)'s support.
* Add support or optional addon for image manipulation (scaling, flip/rotation)
* Add more examples