#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


const int pot1Pin = A1;
const int pot2Pin = A0;
const int switchPin = 4;
const int buzzerPin = 13;

typedef struct {
  int y;
} Paddle;
typedef struct {
  int x, y, dirX, dirY;
} Ball;

const int aiMaxSpeed = 3;
const unsigned long speedInterval = 10000UL;
const unsigned long sizeInterval = 7000UL;
const unsigned long paddleInterval = 12000UL;
const unsigned long gapBetweenStages = 10000UL;
const int difficultyLevels = 5;

unsigned long stage1Duration, stage2Start, stage3Start;

const int speedLevels[difficultyLevels] = {1, 2, 4, 5, 6};
const int ballSizes[difficultyLevels] = {8, 6, 4, 3, 2};
const int paddleSizes[difficultyLevels] = {16, 12, 8, 4, 2};

int currentBallSize;
int currentPaddleHeight;

Paddle paddle1, paddle2;
Ball ball;
int score1 = 0, score2 = 0;

enum { STARTUP, PLAYING, GAME_OVER, WAITING_TO_RESTART } gameState;
enum { SINGLE, DUEL } gameMode;

unsigned long gameStartTime;
unsigned long waitUntil = 0;

void playTone(int freq, int dur) {
  tone(buzzerPin, freq, dur);
}

void resetBall() {
  ball.x = SCREEN_WIDTH / 2;
  ball.y = SCREEN_HEIGHT / 2;
  pinMode(A2, INPUT);
  randomSeed(analogRead(A2));
  ball.dirX = (random(0, 2) == 0) ? -1 : 1;
  ball.dirY = random(-1, 2);
  if (ball.dirY == 0) ball.dirY = 1;
}

void setup() {
  Serial.begin(9600);
  pinMode(A2, INPUT);
  randomSeed(analogRead(A2));
  pinMode(pot1Pin, INPUT);
  pinMode(pot2Pin, INPUT);
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) while (true);
  display.clearDisplay();

  stage1Duration = (difficultyLevels - 1) * speedInterval;
  stage2Start = stage1Duration + gapBetweenStages;
  stage3Start = stage2Start + gapBetweenStages;

  gameMode = (digitalRead(switchPin) == LOW) ? SINGLE : DUEL;

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Welcome to Pong game");
  display.print("Mode: ");
  display.println((gameMode == SINGLE) ? "Single" : "Duel");
  display.display();
  delay(5000);
  playTone(440, 200);
  delay(220);
  playTone(550, 200); 
  delay(220);
  playTone(660, 200);
  delay(220);

  score1 = score2 = 0;
  gameStartTime = millis();
  resetBall();
  gameState = PLAYING;
}

void loop() {
  unsigned long elapsed = millis() - gameStartTime;

  int speedLvl = (elapsed < stage1Duration) ? (elapsed / speedInterval + 1) : difficultyLevels;
  int sizeLvl = (elapsed >= stage2Start) ? min(difficultyLevels, (int)((elapsed - stage2Start) / sizeInterval) + 1) : 1;
  int paddleLvl = (elapsed >= stage3Start) ? min(difficultyLevels, (int)((elapsed - stage3Start) / paddleInterval) + 1) : 1;

  int ballSpeed = speedLevels[speedLvl - 1];
  currentBallSize = ballSizes[sizeLvl - 1];
  currentPaddleHeight = paddleSizes[paddleLvl - 1];

  switch (gameState) {
    case PLAYING:
      updateGame(ballSpeed);
      renderGame();
      if (score1 >= 5 || score2 >= 5) {
        gameState = GAME_OVER;
      }
      break;

    case GAME_OVER:
      display.clearDisplay();
      display.setCursor(0, 0);
      if (gameMode == DUEL) {
        display.print("The Winner is player ");
        display.println((score1 > score2) ? "1" : "2");
      } else {
        display.println((score2 > score1) ? "You lose! too bad, good luck next time" : "Congratulations! You won the most computer!");
      }
      display.display();
      playTone(660, 200); playTone(550, 200); playTone(440, 200);
      waitUntil = millis() + 5000;
      gameState = WAITING_TO_RESTART;
      break;

    case WAITING_TO_RESTART:
      if (millis() > waitUntil) {
        gameMode = (digitalRead(switchPin) == LOW) ? SINGLE : DUEL;
        score1 = score2 = 0;
        gameStartTime = millis();
        resetBall();
        gameState = PLAYING;
      }
      break;
  }
}

void updateGame(int ballSpeed) {
  paddle1.y = map(analogRead(pot1Pin), 0, 1023, 0, SCREEN_HEIGHT - currentPaddleHeight);

  if (gameMode == SINGLE) {
    static unsigned long lastMove = 0;
    static int aiDirection = 1;
    unsigned long aiInterval = 30;
    if (millis() - lastMove > aiInterval) {
      paddle2.y += aiDirection * aiMaxSpeed;
      if (paddle2.y <= 0 || paddle2.y >= SCREEN_HEIGHT - currentPaddleHeight)
        aiDirection *= -1;
      lastMove = millis();
    }
  } else {
    paddle2.y = map(analogRead(pot2Pin), 0, 1023, 0, SCREEN_HEIGHT - currentPaddleHeight);
  }


  paddle1.y = constrain(paddle1.y, 0, SCREEN_HEIGHT - currentPaddleHeight);
  paddle2.y = constrain(paddle2.y, 0, SCREEN_HEIGHT - currentPaddleHeight);

  ball.x += ball.dirX * ballSpeed;
  ball.y += ball.dirY * ballSpeed;

  if (ball.y <= 0 || ball.y >= SCREEN_HEIGHT - currentBallSize) {
    ball.dirY = -ball.dirY;
    playTone(1000, 50);
  }

  if (ball.x <= 2 + currentBallSize && ball.y + currentBallSize >= paddle1.y && ball.y <= paddle1.y + currentPaddleHeight) {
    ball.dirX = 1;
    pinMode(A2, INPUT);
    randomSeed(analogRead(A2));
    ball.dirY = random(-1, 2);
    if (ball.dirY == 0) ball.dirY = 1;
    playTone(800, 50);
  }
  if (ball.x + currentBallSize >= SCREEN_WIDTH - 2 && ball.y + currentBallSize >= paddle2.y && ball.y <= paddle2.y + currentPaddleHeight) {
    ball.dirX = -1;
    // before any random() calls:
    pinMode(A2, INPUT);
    randomSeed(analogRead(A2));
    ball.dirY = random(-1, 2);
    if (ball.dirY == 0) ball.dirY = 1;
    playTone(800, 50);
  }

  if (ball.x + currentBallSize < 0 || (ball.dirX < 0 && ball.x < 4 && (ball.y + currentBallSize < paddle1.y || ball.y > paddle1.y + currentPaddleHeight))) {
    score2++;
    ball.x = 2;
    ball.dirX = 1;
    // before any random() calls:
    pinMode(A2, INPUT);
    randomSeed(analogRead(A2));
    ball.dirY = random(-1, 2);
    if (ball.dirY == 0) ball.dirY = 1;
    playTone(1200, 100);
  }

  if (ball.x > SCREEN_WIDTH || (ball.dirX > 0 && ball.x + currentBallSize > SCREEN_WIDTH - 4 && (ball.y + currentBallSize < paddle2.y || ball.y > paddle2.y + currentPaddleHeight))) {
    score1++;
    ball.x = SCREEN_WIDTH - currentBallSize - 2;
    ball.dirX = -1;
    ball.dirY = random(-1, 2);
    if (ball.dirY == 0) ball.dirY = -1;
    playTone(1200, 100);
  }
}

void renderGame() {
  display.clearDisplay();

  // Draw court with dotted center line
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  for (int i = 4; i < SCREEN_HEIGHT; i += 8) {
    display.drawFastVLine(SCREEN_WIDTH / 2, i, 4, WHITE);
  }

  // Draw paddles with rounded edges
  display.fillRoundRect(2, paddle1.y, 3, currentPaddleHeight, 1, WHITE);
  display.fillRoundRect(SCREEN_WIDTH - 5, paddle2.y, 3, currentPaddleHeight, 1, WHITE);

  // Draw ball with better visibility
  display.fillCircle(ball.x + currentBallSize / 2, ball.y + currentBallSize / 2, currentBallSize / 2, WHITE);

  // Draw scores in bold text with line
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(40, 3); display.print(score1);
  display.setCursor(80, 3); display.print(score2);

  display.display();
}


//better AI implementation with dynamic foe
// if (gameMode==SINGLE) {
//   int aiCenter   = paddle2.y + currentPaddleHeight/2;
//   int ballCenter = ball.y   + currentBallSize/2;
//   int rawMove    = max(1, ballSpeed);
//   int aiMove     = min(rawMove, aiMaxSpeed);
//   if (aiCenter < ballCenter) paddle2.y += aiMove;
//   else if (aiCenter > ballCenter) paddle2.y -= aiMove;
// } else {
//   paddle2.y = map(analogRead(pot2Pin),0,1023,0,SCREEN_HEIGHT-currentPaddleHeight);
// }
