# Wild West Shootout – ESP32 CYD Game 🤠

A fast Wild West touchscreen reaction game for the ESP32 CYD. Spot the bandits in the saloon windows, shoot the bad guys, and avoid the friendly characters.

## Features

- 320×240 Wild West saloon graphics
- XPT2046 touchscreen controls
- Bandit and friendly targets
- Multiple saloon windows with random target spawning
- Score system with penalties for mistakes
- Level-complete and game-over screens
- Buzzer sound effects

## How to play

Tap the window containing a bandit to score points. Do not shoot the friendly characters. Missing a target or hitting a friendly reduces your score.

## Hardware

- ESP32-2432S028 / ESP32 CYD (Cheap Yellow Display)
- 2.8-inch 320×240 ILI9341 display
- XPT2046 resistive touchscreen
- On-board buzzer

## Arduino setup

Open `wild_west.ino` in the Arduino IDE and select **ESP32 Dev Module** (or the matching CYD board profile).

Required libraries:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit ILI9341](https://github.com/adafruit/Adafruit_ILI9341)
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)

`SPI` is provided by the ESP32 Arduino core.

## Display used for the project

👉 [ESP32 touchscreen item link](https://temu.to/k/ea65oy4oun8)

Disclosure: this link may be an affiliate link. It does not change the price for you.

## Video

🎬 [Watch Wild West Shootout on YouTube](https://youtube.com/shorts/H4KeBw8kWzA)

## License

This project is released under the MIT License. See [LICENSE](LICENSE).
