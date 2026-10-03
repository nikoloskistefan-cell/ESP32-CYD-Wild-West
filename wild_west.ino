#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// =====================================================
// TFT
// =====================================================

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21

// =====================================================
// TOUCH
// =====================================================

#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK  25
#define TOUCH_CS   33

#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3900
#define TOUCH_MIN_Y 200
#define TOUCH_MAX_Y 3900

// =====================================================
// BUZZER
// =====================================================

#define BUZZER_PIN 22

// =====================================================
// SCREEN
// =====================================================

#define SCREEN_W 320
#define SCREEN_H 240

// =====================================================
// HARDWARE
// =====================================================

SPIClass tftSPI(HSPI);
SPIClass touchSPI(VSPI);

Adafruit_ILI9341 tft(
  &tftSPI,
  TFT_DC,
  TFT_CS,
  TFT_RST
);

XPT2046_Touchscreen ts(TOUCH_CS);

// =====================================================
// COLORS
// =====================================================

#define SKY_COLOR       0x5DDF
#define SAND_COLOR      0xD5A5
#define WOOD_COLOR      0xA285
#define WOOD_DARK       0x79E0
#define WINDOW_DARK     0x2104
#define SALOON_RED      0xA145
#define SKIN_COLOR      0xFD20
#define FRIEND_COLOR    0x07FF
#define BANDIT_COLOR    0x0010

// =====================================================
// GAME STATES
// =====================================================

enum GameState {
  TITLE_SCREEN,
  PLAYING,
  LEVEL_COMPLETE,
  GAME_OVER
};

GameState gameState = TITLE_SCREEN;

// =====================================================
// TARGET TYPES
// =====================================================

enum TargetType {
  NONE,
  BANDIT,
  FRIENDLY
};

// =====================================================
// WINDOW DATA
// =====================================================

struct WindowSlot {
  int x;
  int y;
  int w;
  int h;
  TargetType target;
  bool active;
};

WindowSlot windows[] = {
  {35,  75, 55, 55, NONE, false},
  {132, 75, 55, 55, NONE, false},
  {230, 75, 55, 55, NONE, false},

  {35,  145, 55, 55, NONE, false},
  {132, 145, 55, 55, NONE, false},
  {230, 145, 55, 55, NONE, false}
};

const int WINDOW_COUNT =
  sizeof(windows) /
  sizeof(windows[0]);

// =====================================================
// GAME DATA
// =====================================================

int level = 1;
long score = 0;

int lives = 3;

int targetsHit = 0;
int targetsNeeded = 8;

// =====================================================
// TARGET TIMING
// =====================================================

int activeWindow = -1;

unsigned long targetShownAt = 0;
unsigned long nextTargetAt = 0;

unsigned long targetVisibleTime = 2200;

unsigned long spawnDelayMin = 800;
unsigned long spawnDelayMax = 1500;

// =====================================================
// TOUCH
// =====================================================

bool touchWasDown = false;

// =====================================================
// TITLE MUSIC
// =====================================================

const int titleNotes[] = {
  392, 523, 659, 784,
  659, 523,

  440, 554, 659, 880,
  659, 554,

  523, 659, 784, 1047,
  784, 659,

  392, 494, 587, 784
};

const int titleDurations[] = {
  150, 150, 150, 300,
  150, 250,

  150, 150, 150, 300,
  150, 250,

  150, 150, 160, 320,
  150, 250,

  160, 160, 160, 350
};

const int TITLE_NOTE_COUNT =
  sizeof(titleNotes) /
  sizeof(titleNotes[0]);

int titleNote = 0;

unsigned long titleNoteStarted = 0;

bool titleMusicPlaying = false;

// =====================================================
// SOUND
// =====================================================

void soundOff() {
  ledcWriteTone(
    BUZZER_PIN,
    0
  );
}

// -----------------------------------------------------
// TITLE
// -----------------------------------------------------

void startTitleMusic() {

  titleNote = 0;

  titleNoteStarted =
    millis();

  titleMusicPlaying =
    true;

  ledcWriteTone(
    BUZZER_PIN,
    titleNotes[0]
  );
}

void stopTitleMusic() {

  titleMusicPlaying =
    false;

  soundOff();
}

void updateTitleMusic() {

  if (!titleMusicPlaying)
    return;

  unsigned long now =
    millis();

  if (
    now - titleNoteStarted >=
    titleDurations[titleNote]
  ) {

    titleNote++;

    if (
      titleNote >= TITLE_NOTE_COUNT
    ) {
      titleNote = 0;
    }

    ledcWriteTone(
      BUZZER_PIN,
      titleNotes[titleNote]
    );

    titleNoteStarted =
      now;
  }
}

// -----------------------------------------------------
// SHOT
// -----------------------------------------------------

void soundShot() {

  ledcWriteTone(
    BUZZER_PIN,
    1800
  );

  delay(15);

  ledcWriteTone(
    BUZZER_PIN,
    900
  );

  delay(20);

  ledcWriteTone(
    BUZZER_PIN,
    250
  );

  delay(35);

  soundOff();
}

// -----------------------------------------------------
// HIT
// -----------------------------------------------------

void soundHit() {

  ledcWriteTone(
    BUZZER_PIN,
    900
  );

  delay(50);

  ledcWriteTone(
    BUZZER_PIN,
    1300
  );

  delay(60);

  ledcWriteTone(
    BUZZER_PIN,
    1800
  );

  delay(90);

  soundOff();
}

// -----------------------------------------------------
// FRIENDLY HIT
// -----------------------------------------------------

void soundFriendlyHit() {

  ledcWriteTone(
    BUZZER_PIN,
    350
  );

  delay(100);

  ledcWriteTone(
    BUZZER_PIN,
    220
  );

  delay(120);

  ledcWriteTone(
    BUZZER_PIN,
    140
  );

  delay(180);

  soundOff();
}

// -----------------------------------------------------
// LEVEL COMPLETE
// -----------------------------------------------------

void soundLevelComplete() {

  int notes[] = {
    523,
    659,
    784,
    1047,
    1319
  };

  for (
    int i = 0;
    i < 5;
    i++
  ) {

    ledcWriteTone(
      BUZZER_PIN,
      notes[i]
    );

    delay(100);
  }

  soundOff();
}

// -----------------------------------------------------
// GAME OVER
// -----------------------------------------------------

void soundGameOver() {

  ledcWriteTone(
    BUZZER_PIN,
    500
  );

  delay(120);

  ledcWriteTone(
    BUZZER_PIN,
    350
  );

  delay(150);

  ledcWriteTone(
    BUZZER_PIN,
    220
  );

  delay(200);

  ledcWriteTone(
    BUZZER_PIN,
    120
  );

  delay(350);

  soundOff();
}

// =====================================================
// TEXT
// =====================================================

void centerText(
  const char* text,
  int y,
  int size,
  uint16_t color
) {

  tft.setTextSize(size);
  tft.setTextColor(color);

  int16_t x1;
  int16_t y1;

  uint16_t w;
  uint16_t h;

  tft.getTextBounds(
    text,
    0,
    y,
    &x1,
    &y1,
    &w,
    &h
  );

  tft.setCursor(
    (SCREEN_W - w) / 2,
    y
  );

  tft.print(text);
}

// =====================================================
// TITLE SCREEN
// =====================================================

void drawTitleScreen() {

  tft.fillScreen(
    SKY_COLOR
  );

  // sand
  tft.fillRect(
    0,
    170,
    320,
    70,
    SAND_COLOR
  );

  // saloon
  tft.fillRect(
    55,
    70,
    210,
    100,
    WOOD_COLOR
  );

  tft.fillRect(
    80,
    45,
    160,
    35,
    WOOD_DARK
  );

  tft.fillRect(
    45,
    65,
    230,
    8,
    ILI9341_BLACK
  );

  // doors
  tft.fillRect(
    135,
    115,
    50,
    55,
    SALOON_RED
  );

  tft.drawLine(
    160,
    115,
    160,
    170,
    ILI9341_BLACK
  );

  // windows
  tft.fillRect(
    75,
    95,
    40,
    35,
    WINDOW_DARK
  );

  tft.fillRect(
    205,
    95,
    40,
    35,
    WINDOW_DARK
  );

  centerText(
    "WILD WEST",
    12,
    3,
    ILI9341_YELLOW
  );

  centerText(
    "SHOOTOUT",
    40,
    2,
    ILI9341_RED
  );

  centerText(
    "BANDITS = SHOOT",
    185,
    1,
    ILI9341_BLACK
  );

  centerText(
    "FRIENDS = DON'T SHOOT",
    198,
    1,
    ILI9341_BLACK
  );

  tft.fillRoundRect(
    90,
    215,
    140,
    20,
    5,
    ILI9341_DARKGREEN
  );

  centerText(
    "TAP TO START",
    221,
    1,
    ILI9341_WHITE
  );
}

// =====================================================
// WESTERN BACKGROUND
// =====================================================

void drawBackground() {

  tft.fillScreen(
    SKY_COLOR
  );

  tft.fillRect(
    0,
    205,
    320,
    35,
    SAND_COLOR
  );

  // saloon wall
  tft.fillRect(
    18,
    55,
    284,
    150,
    WOOD_COLOR
  );

  // planks
  for (
    int y = 60;
    y < 205;
    y += 15
  ) {

    tft.drawLine(
      18,
      y,
      302,
      y,
      WOOD_DARK
    );
  }

  // sign
  tft.fillRect(
    75,
    30,
    170,
    32,
    WOOD_DARK
  );

  centerText(
    "SALOON",
    40,
    2,
    ILI9341_YELLOW
  );

  // roof
  tft.fillRect(
    10,
    52,
    300,
    8,
    ILI9341_BLACK
  );

  // windows
  for (
    int i = 0;
    i < WINDOW_COUNT;
    i++
  ) {

    int x =
      windows[i].x;

    int y =
      windows[i].y;

    int w =
      windows[i].w;

    int h =
      windows[i].h;

    tft.fillRect(
      x - 4,
      y - 4,
      w + 8,
      h + 8,
      WOOD_DARK
    );

    tft.fillRect(
      x,
      y,
      w,
      h,
      WINDOW_DARK
    );

    tft.drawLine(
      x + w / 2,
      y,
      x + w / 2,
      y + h,
      WOOD_COLOR
    );
  }
}

// =====================================================
// HUD
// =====================================================

void drawHUD() {

  tft.fillRect(
    0,
    0,
    320,
    27,
    ILI9341_BLACK
  );

  tft.setTextSize(1);

  tft.setTextColor(
    ILI9341_WHITE
  );

  tft.setCursor(
    5,
    5
  );

  tft.print(
    "LEVEL "
  );

  tft.print(
    level
  );

  tft.setCursor(
    90,
    5
  );

  tft.print(
    "SCORE "
  );

  tft.print(
    score
  );

  tft.setCursor(
    210,
    5
  );

  tft.print(
    "LIVES "
  );

  for (
    int i = 0;
    i < lives;
    i++
  ) {

    tft.setTextColor(
      ILI9341_RED
    );

    tft.print("*");
  }

  tft.setTextColor(
    ILI9341_YELLOW
  );

  tft.setCursor(
    5,
    17
  );

  tft.print(
    "HITS "
  );

  tft.print(
    targetsHit
  );

  tft.print(
    "/"
  );

  tft.print(
    targetsNeeded
  );
}

// =====================================================
// BANDIT
// =====================================================

void drawBandit(
  int windowIndex
) {

  WindowSlot &w =
    windows[windowIndex];

  int cx =
    w.x + w.w / 2;

  int cy =
    w.y + w.h / 2 + 3;

  // hat
  tft.fillRect(
    cx - 16,
    cy - 22,
    32,
    5,
    ILI9341_BLACK
  );

  tft.fillRect(
    cx - 10,
    cy - 30,
    20,
    10,
    ILI9341_BLACK
  );

  // face
  tft.fillCircle(
    cx,
    cy - 12,
    10,
    SKIN_COLOR
  );

  // eyes
  tft.fillCircle(
    cx - 4,
    cy - 14,
    2,
    ILI9341_BLACK
  );

  tft.fillCircle(
    cx + 4,
    cy - 14,
    2,
    ILI9341_BLACK
  );

  // bandana
  tft.fillRect(
    cx - 9,
    cy - 8,
    18,
    5,
    ILI9341_RED
  );

  // body
  tft.fillRect(
    cx - 12,
    cy,
    24,
    20,
    BANDIT_COLOR
  );

  // gun
  tft.drawLine(
    cx + 10,
    cy + 5,
    cx + 20,
    cy,
    ILI9341_DARKGREY
  );

  tft.fillCircle(
    cx + 21,
    cy,
    2,
    ILI9341_BLACK
  );
}

// =====================================================
// FRIENDLY
// =====================================================

void drawFriendly(
  int windowIndex
) {

  WindowSlot &w =
    windows[windowIndex];

  int cx =
    w.x + w.w / 2;

  int cy =
    w.y + w.h / 2 + 3;

  // hat
  tft.fillRect(
    cx - 15,
    cy - 22,
    30,
    4,
    ILI9341_YELLOW
  );

  tft.fillRect(
    cx - 9,
    cy - 29,
    18,
    9,
    ILI9341_YELLOW
  );

  // face
  tft.fillCircle(
    cx,
    cy - 11,
    10,
    SKIN_COLOR
  );

  // eyes
  tft.fillCircle(
    cx - 4,
    cy - 13,
    2,
    ILI9341_BLACK
  );

  tft.fillCircle(
    cx + 4,
    cy - 13,
    2,
    ILI9341_BLACK
  );

  // smile
  tft.drawLine(
    cx - 4,
    cy - 6,
    cx + 4,
    cy - 6,
    ILI9341_BLACK
  );

  // body
  tft.fillRect(
    cx - 12,
    cy,
    24,
    20,
    FRIEND_COLOR
  );

  // arms
  tft.drawLine(
    cx - 10,
    cy + 3,
    cx - 20,
    cy - 8,
    ILI9341_WHITE
  );

  tft.drawLine(
    cx + 10,
    cy + 3,
    cx + 20,
    cy - 8,
    ILI9341_WHITE
  );
}

// =====================================================
// CLEAR WINDOW
// =====================================================

void clearWindow(
  int index
) {

  WindowSlot &w =
    windows[index];

  tft.fillRect(
    w.x,
    w.y,
    w.w,
    w.h,
    WINDOW_DARK
  );

  tft.drawLine(
    w.x + w.w / 2,
    w.y,
    w.x + w.w / 2,
    w.y + w.h,
    WOOD_COLOR
  );
}

// =====================================================
// SPAWN TARGET
// =====================================================

void spawnTarget() {

  // safety cleanup
  if (
    activeWindow >= 0
  ) {

    clearWindow(
      activeWindow
    );

    windows[
      activeWindow
    ].active = false;
  }

  activeWindow =
    random(
      0,
      WINDOW_COUNT
    );

  WindowSlot &w =
    windows[activeWindow];

  w.active =
    true;

  // 25% friendly
  if (
    random(0, 100) < 25
  ) {

    w.target =
      FRIENDLY;

    drawFriendly(
      activeWindow
    );

  } else {

    w.target =
      BANDIT;

    drawBandit(
      activeWindow
    );
  }

  // IMPORTANT:
  // take timestamp AFTER drawing target
  targetShownAt =
    millis();

  Serial.print(
    "TARGET spawned window "
  );

  Serial.print(
    activeWindow
  );

  Serial.print(
    " type="
  );

  if (
    w.target == BANDIT
  ) {

    Serial.println(
      "BANDIT"
    );

  } else {

    Serial.println(
      "FRIENDLY"
    );
  }
}

// =====================================================
// NEXT TARGET
// =====================================================

void scheduleNextTarget() {

  activeWindow =
    -1;

  unsigned long delayTime =
    random(
      spawnDelayMin,
      spawnDelayMax
    );

  nextTargetAt =
    millis() +
    delayTime;
}

// =====================================================
// DIFFICULTY
// =====================================================

void updateDifficulty() {

  // -----------------------------------------------
  // TARGET VISIBLE TIME
  // -----------------------------------------------

  targetVisibleTime =
    2200 -
    (level - 1) * 150;

  if (
    targetVisibleTime < 650
  ) {

    targetVisibleTime =
      650;
  }

  // -----------------------------------------------
  // SPAWN MIN
  // -----------------------------------------------

  spawnDelayMin =
    800 -
    (level - 1) * 50;

  if (
    spawnDelayMin < 200
  ) {

    spawnDelayMin =
      200;
  }

  // -----------------------------------------------
  // SPAWN MAX
  // -----------------------------------------------

  spawnDelayMax =
    1500 -
    (level - 1) * 70;

  if (
    spawnDelayMax < 450
  ) {

    spawnDelayMax =
      450;
  }

  // important safety:
  // max must remain > min

  if (
    spawnDelayMax <=
    spawnDelayMin
  ) {

    spawnDelayMax =
      spawnDelayMin + 200;
  }

  // -----------------------------------------------
  // TARGETS NEEDED
  // -----------------------------------------------

  targetsNeeded =
    7 + level;

  if (
    targetsNeeded > 15
  ) {

    targetsNeeded =
      15;
  }

  Serial.print(
    "LEVEL "
  );

  Serial.print(
    level
  );

  Serial.print(
    " visible="
  );

  Serial.print(
    targetVisibleTime
  );

  Serial.print(
    " spawn="
  );

  Serial.print(
    spawnDelayMin
  );

  Serial.print(
    "-"
  );

  Serial.println(
    spawnDelayMax
  );
}

// =====================================================
// TOUCH
// =====================================================

bool getNewTap(
  int &screenX,
  int &screenY
) {

  TS_Point p =
    ts.getPoint();

  bool down =
    p.z > 200;

  if (
    !down
  ) {

    touchWasDown =
      false;

    return false;
  }

  if (
    touchWasDown
  ) {

    return false;
  }

  touchWasDown =
    true;

  screenX =
    map(
      p.x,
      TOUCH_MIN_X,
      TOUCH_MAX_X,
      0,
      SCREEN_W
    );

  screenY =
    map(
      p.y,
      TOUCH_MIN_Y,
      TOUCH_MAX_Y,
      0,
      SCREEN_H
    );

  screenX =
    constrain(
      screenX,
      0,
      SCREEN_W - 1
    );

  screenY =
    constrain(
      screenY,
      0,
      SCREEN_H - 1
    );

  Serial.print(
    "TAP X="
  );

  Serial.print(
    screenX
  );

  Serial.print(
    " Y="
  );

  Serial.println(
    screenY
  );

  return true;
}

// =====================================================
// FIND WINDOW
// =====================================================

int findTouchedWindow(
  int x,
  int y
) {

  for (
    int i = 0;
    i < WINDOW_COUNT;
    i++
  ) {

    WindowSlot &w =
      windows[i];

    if (
      x >= w.x &&
      x <= w.x + w.w &&
      y >= w.y &&
      y <= w.y + w.h
    ) {

      return i;
    }
  }

  return -1;
}

// =====================================================
// HANDLE SHOT
// =====================================================

void handleShot(
  int tapX,
  int tapY
) {

  soundShot();

  int windowIndex =
    findTouchedWindow(
      tapX,
      tapY
    );

  // -----------------------------------------------
  // SHOT OUTSIDE WINDOW
  // -----------------------------------------------

  if (
    windowIndex < 0
  ) {

    score -= 10;

    if (
      score < 0
    ) {
      score = 0;
    }

    drawHUD();

    return;
  }

  WindowSlot &w =
    windows[windowIndex];

  // -----------------------------------------------
  // EMPTY WINDOW
  // -----------------------------------------------

  if (
    !w.active
  ) {

    score -= 10;

    if (
      score < 0
    ) {
      score = 0;
    }

    drawHUD();

    return;
  }

  // -----------------------------------------------
  // BANDIT HIT
  // -----------------------------------------------

  if (
    w.target ==
    BANDIT
  ) {

    soundHit();

    score +=
      100 +
      level * 20;

    targetsHit++;

    clearWindow(
      windowIndex
    );

    w.active =
      false;

    w.target =
      NONE;

    activeWindow =
      -1;

    drawHUD();

    if (
      targetsHit >=
      targetsNeeded
    ) {

      gameState =
        LEVEL_COMPLETE;

      return;
    }

    scheduleNextTarget();

    return;
  }

  // -----------------------------------------------
  // FRIENDLY HIT
  // -----------------------------------------------

  if (
    w.target ==
    FRIENDLY
  ) {

    soundFriendlyHit();

    lives--;

    score -= 150;

    if (
      score < 0
    ) {
      score = 0;
    }

    clearWindow(
      windowIndex
    );

    w.active =
      false;

    w.target =
      NONE;

    activeWindow =
      -1;

    drawHUD();

    if (
      lives <= 0
    ) {

      gameState =
        GAME_OVER;

      return;
    }

    scheduleNextTarget();
  }
}

// =====================================================
// MISSED TARGET
// =====================================================

void missedTarget() {

  if (
    activeWindow < 0
  ) {

    return;
  }

  WindowSlot &w =
    windows[activeWindow];

  Serial.print(
    "TARGET timeout type="
  );

  if (
    w.target == BANDIT
  ) {

    Serial.println(
      "BANDIT"
    );

    lives--;

  } else {

    Serial.println(
      "FRIENDLY"
    );
  }

  int oldWindow =
    activeWindow;

  clearWindow(
    oldWindow
  );

  w.active =
    false;

  w.target =
    NONE;

  activeWindow =
    -1;

  drawHUD();

  if (
    lives <= 0
  ) {

    gameState =
      GAME_OVER;

    return;
  }

  scheduleNextTarget();
}

// =====================================================
// LEVEL COMPLETE SCREEN
// =====================================================

void showLevelComplete() {

  soundLevelComplete();

  tft.fillRoundRect(
    55,
    85,
    210,
    80,
    10,
    ILI9341_BLACK
  );

  tft.drawRoundRect(
    55,
    85,
    210,
    80,
    10,
    ILI9341_YELLOW
  );

  centerText(
    "LEVEL CLEAR!",
    102,
    2,
    ILI9341_GREEN
  );

  centerText(
    "TAP FOR NEXT LEVEL",
    140,
    1,
    ILI9341_WHITE
  );
}

// =====================================================
// GAME OVER SCREEN
// =====================================================

void showGameOver() {

  soundGameOver();

  tft.fillScreen(
    ILI9341_BLACK
  );

  centerText(
    "GAME OVER",
    55,
    3,
    ILI9341_RED
  );

  centerText(
    "FINAL SCORE",
    105,
    2,
    ILI9341_WHITE
  );

  char buffer[20];

  sprintf(
    buffer,
    "%ld",
    score
  );

  centerText(
    buffer,
    135,
    3,
    ILI9341_YELLOW
  );

  centerText(
    "TAP TO RESTART",
    205,
    1,
    ILI9341_WHITE
  );
}

// =====================================================
// START LEVEL
// =====================================================

void startLevel() {

  targetsHit =
    0;

  activeWindow =
    -1;

  // reset all window states

  for (
    int i = 0;
    i < WINDOW_COUNT;
    i++
  ) {

    windows[i].active =
      false;

    windows[i].target =
      NONE;
  }

  updateDifficulty();

  drawBackground();

  drawHUD();

  nextTargetAt =
    millis() +
    800;

  gameState =
    PLAYING;
}

// =====================================================
// NEW GAME
// =====================================================

void newGame() {

  level =
    1;

  score =
    0;

  lives =
    3;

  targetsHit =
    0;

  startLevel();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );

  Serial.println();
  Serial.println(
    "Wild West booting..."
  );

  // buzzer

  ledcAttach(
    BUZZER_PIN,
    2000,
    8
  );

  soundOff();

  // backlight

  pinMode(
    TFT_BL,
    OUTPUT
  );

  digitalWrite(
    TFT_BL,
    HIGH
  );

  // TFT

  tftSPI.begin(
    TFT_SCLK,
    TFT_MISO,
    TFT_MOSI,
    TFT_CS
  );

  tft.begin();

  tft.setRotation(
    1
  );

  tft.setTextWrap(
    false
  );

  // touch

  touchSPI.begin(
    TOUCH_CLK,
    TOUCH_MISO,
    TOUCH_MOSI,
    TOUCH_CS
  );

  ts.begin(
    touchSPI
  );

  ts.setRotation(
    1
  );

  randomSeed(
    micros()
  );

  drawTitleScreen();

  startTitleMusic();

  Serial.println(
    "Wild West READY"
  );
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  unsigned long now =
    millis();

  // ===================================================
  // TITLE MUSIC
  // ===================================================

  if (
    gameState ==
    TITLE_SCREEN
  ) {

    updateTitleMusic();
  }

  // ===================================================
  // TOUCH
  // ===================================================

  int tapX;
  int tapY;

  if (
    getNewTap(
      tapX,
      tapY
    )
  ) {

    // -----------------------------------------------
    // TITLE
    // -----------------------------------------------

    if (
      gameState ==
      TITLE_SCREEN
    ) {

      stopTitleMusic();

      newGame();

      delay(150);

      return;
    }

    // -----------------------------------------------
    // PLAYING
    // -----------------------------------------------

    if (
      gameState ==
      PLAYING
    ) {

      handleShot(
        tapX,
        tapY
      );

      return;
    }

    // -----------------------------------------------
    // LEVEL COMPLETE
    // -----------------------------------------------

    if (
      gameState ==
      LEVEL_COMPLETE
    ) {

      level++;

      startLevel();

      delay(150);

      return;
    }

    // -----------------------------------------------
    // GAME OVER
    // -----------------------------------------------

    if (
      gameState ==
      GAME_OVER
    ) {

      drawTitleScreen();

      gameState =
        TITLE_SCREEN;

      startTitleMusic();

      delay(150);

      return;
    }
  }

  // ===================================================
  // PLAYING
  // ===================================================

  if (
    gameState ==
    PLAYING
  ) {

    // -----------------------------------------------
    // SPAWN TARGET
    // -----------------------------------------------

    if (
      activeWindow < 0 &&
      millis() >= nextTargetAt
    ) {

      spawnTarget();

      // CRITICAL FIX:
      // Do not execute timeout check during
      // same loop iteration in which target appeared.
      return;
    }

    // -----------------------------------------------
    // TARGET TIMEOUT
    // -----------------------------------------------

    if (
      activeWindow >= 0
    ) {

      unsigned long currentTime =
        millis();

      if (
        currentTime -
        targetShownAt >=
        targetVisibleTime
      ) {

        missedTarget();

        return;
      }
    }
  }

  // ===================================================
  // LEVEL COMPLETE
  // ===================================================

  if (
    gameState ==
    LEVEL_COMPLETE
  ) {

    showLevelComplete();

    // wait for tap without redrawing every frame

    while (
      gameState ==
      LEVEL_COMPLETE
    ) {

      int x;
      int y;

      if (
        getNewTap(
          x,
          y
        )
      ) {

        level++;

        startLevel();

        break;
      }

      delay(5);
    }
  }

  // ===================================================
  // GAME OVER
  // ===================================================

  if (
    gameState ==
    GAME_OVER
  ) {

    showGameOver();

    while (
      gameState ==
      GAME_OVER
    ) {

      int x;
      int y;

      if (
        getNewTap(
          x,
          y
        )
      ) {

        drawTitleScreen();

        gameState =
          TITLE_SCREEN;

        startTitleMusic();

        break;
      }

      delay(5);
    }
  }

  delay(2);
}
