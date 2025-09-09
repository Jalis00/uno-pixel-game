#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

// --- Matrisstorlek ---
const int WIDTH  = 12;
const int HEIGHT = 8;

// Spelare
int playerX = WIDTH / 2;
const int playerY = HEIGHT - 1;

// Spelstatus
uint8_t frame[HEIGHT][WIDTH];
int obstacles[HEIGHT];           
bool gameOver = false;
unsigned long lastStepMs = 0;

// --- Långsammare fall + mjukare svårighetsökning ---
int fallDelayMs = 800;           
const int minFallDelayMs = 300;  
int score = 0;

// ---------- Hjälpfunktioner ----------
void clearFrame() {
  for (int y = 0; y < HEIGHT; y++)
    for (int x = 0; x < WIDTH; x++)
      frame[y][x] = 0;
}

inline void setPix(int x, int y) {
  if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) frame[y][x] = 1;
}

void renderFrame() {
  matrix.renderBitmap(frame, HEIGHT, WIDTH);
}

void renderGame() {
  clearFrame();
  // Rita hinder
  for (int y = 0; y < HEIGHT; y++) {
    if (obstacles[y] >= 0) {
      int x = obstacles[y];
      if (x >= 0 && x < WIDTH) frame[y][x] = 1;
    }
  }
  // Rita spelare
  frame[playerY][playerX] = 1;
  renderFrame();
}

void spawnObstacleTop() {
  // Mindre hinder: 30% chans per steg
  if (random(100) < 30) obstacles[0] = random(WIDTH);
  else obstacles[0] = -1;
}

void stepObstacles() {
  for (int y = HEIGHT - 1; y > 0; y--) {
    obstacles[y] = obstacles[y - 1];
  }
  spawnObstacleTop();
}

bool checkCollision() {
  return (obstacles[playerY] == playerX);
}

void speedUp() {
  // Mjukare svårighetsökning
  if (score % 10 == 0 && fallDelayMs > minFallDelayMs) {
    fallDelayMs -= 10;
    if (fallDelayMs < minFallDelayMs) fallDelayMs = minFallDelayMs;
  }
}

// ---------- Game Over-animation ----------
void drawXAnimation() {
  for (int i = 0; i < 3; i++) {
    clearFrame();
    for (int y = 0; y < HEIGHT; y++) {
      int x1 = (WIDTH - 1) * y / (HEIGHT - 1);  
      int x2 = (WIDTH - 1) - x1;                
      setPix(x1, y);
      setPix(x2, y);
    }
    renderFrame();
    delay(200);
    matrix.clear();
    delay(150);
  }
}

// ====== 3x5-bokstäver för GAME OVER ======
const uint8_t CH_G[5] = {0b111,0b100,0b101,0b101,0b111};
const uint8_t CH_A[5] = {0b010,0b101,0b111,0b101,0b101};
const uint8_t CH_M[5] = {0b101,0b111,0b101,0b101,0b101};
const uint8_t CH_E[5] = {0b111,0b100,0b111,0b100,0b111};
const uint8_t CH_O[5] = {0b111,0b101,0b101,0b101,0b111};
const uint8_t CH_V[5] = {0b101,0b101,0b101,0b101,0b010};
const uint8_t CH_R[5] = {0b110,0b101,0b110,0b101,0b101};

const uint8_t* glyphFor(char c) {
  switch (c) {
    case 'G': return CH_G;
    case 'A': return CH_A;
    case 'M': return CH_M;
    case 'E': return CH_E;
    case 'O': return CH_O;
    case 'V': return CH_V;
    case 'R': return CH_R;
    default:  return nullptr;
  }
}

// Rita en bokstav 3x5
void drawChar3x5(char c, int x0, int y0) {
  if (c == ' ') return;
  const uint8_t* g = glyphFor(c);
  if (!g) return;
  for (int row = 0; row < 5; row++) {
    uint8_t bits = g[row];
    for (int col = 0; col < 3; col++) {
      if (bits & (1 << (2 - col))) {
        setPix(x0 + col, y0 + row);
      }
    }
  }
}

// Rita en textsträng
void drawString3x5(const char* s, int xOffset, int yTop) {
  int x = xOffset;
  for (int i = 0; s[i]; i++) {
    drawChar3x5(s[i], x, yTop);
    x += 4; 
  }
}

// Scrolla text manuellt
void scrollText(const char* s, int yTop, int speedMs) {
  int n = 0; while (s[n]) n++;
  int totalW = (n == 0) ? 0 : (n * 4 - 1);
  for (int x = WIDTH; x > -totalW; x--) {
    clearFrame();
    drawString3x5(s, x, yTop);
    renderFrame();
    delay(speedMs);
  }
}

// Game Over-sekvensen
void gameOverSequence() {
  drawXAnimation();
  scrollText(" GAME ", 1, 90);
  scrollText(" OVER ", 1, 90);
  delay(300);
}

void resetGame() {
  for (int y = 0; y < HEIGHT; y++) obstacles[y] = -1;
  playerX = WIDTH / 2;
  score = 0;
  fallDelayMs = 800;
  gameOver = false;
  lastStepMs = millis();
  renderGame();
}

// ---------- Setup / Loop ----------
void setup() {
  matrix.begin();
  matrix.clear();
  Serial.begin(115200);
  randomSeed(analogRead(A0));
  resetGame();
}

void loop() {
  // Seriell styrning om du vill köra via PC
  while (Serial.available()) {
    char c = Serial.read();
    if (!gameOver) {
      if (c == 'L' && playerX > 0)                { playerX--; renderGame(); }
      else if (c == 'R' && playerX < WIDTH - 1)   { playerX++; renderGame(); }
      else if (c == 'C')                          { playerX = WIDTH/2; renderGame(); }
    }
    if (c == 'S') { resetGame(); }
  }

  // Spel-logiken
  unsigned long now = millis();
  if (!gameOver && now - lastStepMs >= (unsigned long)fallDelayMs) {
    lastStepMs = now;
    stepObstacles();
    if (checkCollision()) {
      gameOver = true;
      renderGame();
      gameOverSequence();
      resetGame();
    } else {
      score++;
      speedUp();
      renderGame();
    }
  }
}
